/***********************************************************************************************************************
 *   SBP Shots - Screenshot automation tool for sub-bottom profiler monitoring during oceanographic surveys.           *
 *                                                                                                                     *
 *   Copyright (C) 2022-2026 Angel Vera Herrera <avera@roa.es>                                                         *
 *                           Real Instituto y Observatorio de la Armada (ROA)                                          *
 *                                                                                                                     *
 *   This file is part of SBP Shots.                                                                                   *
 *                                                                                                                     *
 *   SBP Shots is free software: you can redistribute it and/or modify it under the terms of the GNU General           *
 *   Public License as published by the Free Software Foundation, either version 3 of the License, or (at your         *
 *   option) any later version.                                                                                        *
 *                                                                                                                     *
 *   SBP Shots is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the           *
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License       *
 *   for more details.                                                                                                 *
 *                                                                                                                     *
 *   You should have received a copy of the GNU General Public License along with SBP Shots.                           *
 *   If not, see <http://www.gnu.org/licenses/>.                                                                       *
 **********************************************************************************************************************/

/** ********************************************************************************************************************
 * @file    window_capture_wgc.cpp
 * @brief   Window capture backend using Windows Graphics Capture with Win32 fallbacks.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/capture/window_capture_wgc.h"

#ifdef Q_OS_WIN

#include <QDebug>
#include <QImage>
#include <QRect>
#include <QString>
#include <QtGlobal>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

#include <windows.h>
#include <dwmapi.h>
#include <wingdi.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <unknwn.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

namespace {

using winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame;
using winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool;
using winrt::Windows::Graphics::Capture::GraphicsCaptureItem;
using winrt::Windows::Graphics::Capture::GraphicsCaptureSession;
using winrt::Windows::Graphics::DirectX::DirectXPixelFormat;
using winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice;

constexpr UINT kPrintWindowRenderFullContent = 0x00000002U;

struct CaptureState
{
    std::mutex mutex;
    std::condition_variable condition;
    bool finished = false;
    QString error;
    QImage image;
};

QString hresultHex(HRESULT hr)
{
    return QStringLiteral("0x%1")
        .arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0'));
}

QString hresultToString(const winrt::hresult_error &error)
{
    return QStringLiteral("%1 - %2")
        .arg(hresultHex(static_cast<HRESULT>(error.code())))
        .arg(QString::fromWCharArray(error.message().c_str()));
}

void ensureWinRtInitialized()
{
    static std::once_flag once;

    std::call_once(once,
                   []()
                   {
                       try
                       {
                           winrt::init_apartment(winrt::apartment_type::multi_threaded);
                       }
                       catch (const winrt::hresult_error &error)
                       {
                           /*
                            * Qt, COM, or another component may already have initialized
                            * the current thread with another apartment model. In that case,
                            * do not hard-fail here. The call site will still report WGC
                            * errors if the runtime cannot be used correctly.
                            */
                           if (error.code() != winrt::hresult(RPC_E_CHANGED_MODE))
                               throw;
                       }
                   });
}

bool imageLooksEmpty(const QImage &image)
{
    if (image.isNull())
        return true;

    const int sampleStepY = qMax(1, image.height() / 64);
    const int sampleStepX = qMax(1, image.width() / 64);

    quint64 energy = 0;
    int count = 0;

    for (int y = 0; y < image.height(); y += sampleStepY)
    {
        const auto *line =
            reinterpret_cast<const QRgb *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); x += sampleStepX)
        {
            const QRgb px = line[x];

            energy += static_cast<quint64>(qRed(px))
                    + static_cast<quint64>(qGreen(px))
                    + static_cast<quint64>(qBlue(px));

            ++count;
        }
    }

    return count == 0 || energy == 0;
}

// =====================================================================================================================
// Win32 fallback helpers
// =====================================================================================================================

QRect nativeWindowRect(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return QRect();

    RECT nativeRect = {};

    const HRESULT dwmResult = DwmGetWindowAttribute(hwnd,
                                                    DWMWA_EXTENDED_FRAME_BOUNDS,
                                                    &nativeRect,
                                                    sizeof(nativeRect));

    if (SUCCEEDED(dwmResult))
    {
        const int width = nativeRect.right - nativeRect.left;
        const int height = nativeRect.bottom - nativeRect.top;

        if (width > 0 && height > 0)
            return QRect(nativeRect.left, nativeRect.top, width, height);
    }

    if (!GetWindowRect(hwnd, &nativeRect))
        return QRect();

    const int width = nativeRect.right - nativeRect.left;
    const int height = nativeRect.bottom - nativeRect.top;

    if (width <= 0 || height <= 0)
        return QRect();

    return QRect(nativeRect.left, nativeRect.top, width, height);
}

void forceOpaque(QImage *image)
{
    if (!image || image->isNull())
        return;

    if (image->format() != QImage::Format_ARGB32)
        *image = image->convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < image->height(); ++y)
    {
        auto *line = reinterpret_cast<QRgb *>(image->scanLine(y));

        for (int x = 0; x < image->width(); ++x)
        {
            const QRgb px = line[x];
            line[x] = qRgb(qRed(px), qGreen(px), qBlue(px));
        }
    }
}

bool prepareWindowForFallback(HWND hwnd, QString *errorMessage)
{
    if (!hwnd || !IsWindow(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid target window handle.");

        return false;
    }

    if (!IsWindowVisible(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window is not visible.");

        return false;
    }

    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
        Sleep(250);
    }

    if (IsIconic(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window is minimized and could not be restored.");

        return false;
    }

    return true;
}

QImage makeImageFromDibBits(void *bits,
                            int width,
                            int height,
                            int stride)
{
    if (!bits || width <= 0 || height <= 0 || stride <= 0)
        return QImage();

    QImage image(reinterpret_cast<uchar *>(bits),
                 width,
                 height,
                 stride,
                 QImage::Format_ARGB32);

    QImage copy = image.copy();
    forceOpaque(&copy);

    return copy;
}

QImage captureWindowWithPrintWindow(HWND hwnd,
                                    QString *errorMessage)
{
    QString prepareError;

    if (!prepareWindowForFallback(hwnd, &prepareError))
    {
        if (errorMessage)
            *errorMessage = prepareError;

        return QImage();
    }

    const QRect rect = nativeWindowRect(hwnd);

    if (!rect.isValid() || rect.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window geometry is invalid.");

        return QImage();
    }

    const int width = rect.width();
    const int height = rect.height();

    HDC screenDc = GetDC(nullptr);

    if (!screenDc)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("GetDC(nullptr) failed.");

        return QImage();
    }

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height; // top-down DIB
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;

    HBITMAP bitmap = CreateDIBSection(screenDc,
                                      &bitmapInfo,
                                      DIB_RGB_COLORS,
                                      &bits,
                                      nullptr,
                                      0);

    if (!bitmap || !bits)
    {
        ReleaseDC(nullptr, screenDc);

        if (errorMessage)
            *errorMessage = QStringLiteral("CreateDIBSection failed.");

        return QImage();
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);

    if (!memoryDc)
    {
        DeleteObject(bitmap);
        ReleaseDC(nullptr, screenDc);

        if (errorMessage)
            *errorMessage = QStringLiteral("CreateCompatibleDC failed.");

        return QImage();
    }

    HGDIOBJ oldObject = SelectObject(memoryDc, bitmap);

    RECT fillRect = {0, 0, width, height};
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(memoryDc, &fillRect, blackBrush);
    DeleteObject(blackBrush);

    BOOL ok = PrintWindow(hwnd,
                          memoryDc,
                          kPrintWindowRenderFullContent);

    if (!ok)
        ok = PrintWindow(hwnd, memoryDc, 0);

    QImage image;

    if (ok)
        image = makeImageFromDibBits(bits, width, height, width * 4);

    SelectObject(memoryDc, oldObject);
    DeleteDC(memoryDc);
    DeleteObject(bitmap);
    ReleaseDC(nullptr, screenDc);

    if (!ok)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("PrintWindow failed.");

        return QImage();
    }

    if (imageLooksEmpty(image))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("PrintWindow produced an empty frame.");

        return QImage();
    }

    return image;
}

QImage captureVisibleWindowRectWithBitBlt(HWND hwnd,
                                          QString *errorMessage)
{
    QString prepareError;

    if (!prepareWindowForFallback(hwnd, &prepareError))
    {
        if (errorMessage)
            *errorMessage = prepareError;

        return QImage();
    }

    const QRect rect = nativeWindowRect(hwnd);

    if (!rect.isValid() || rect.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window geometry is invalid.");

        return QImage();
    }

    const int width = rect.width();
    const int height = rect.height();

    HDC screenDc = GetDC(nullptr);

    if (!screenDc)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("GetDC(nullptr) failed.");

        return QImage();
    }

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height; // top-down DIB
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;

    HBITMAP bitmap = CreateDIBSection(screenDc,
                                      &bitmapInfo,
                                      DIB_RGB_COLORS,
                                      &bits,
                                      nullptr,
                                      0);

    if (!bitmap || !bits)
    {
        ReleaseDC(nullptr, screenDc);

        if (errorMessage)
            *errorMessage = QStringLiteral("CreateDIBSection failed.");

        return QImage();
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);

    if (!memoryDc)
    {
        DeleteObject(bitmap);
        ReleaseDC(nullptr, screenDc);

        if (errorMessage)
            *errorMessage = QStringLiteral("CreateCompatibleDC failed.");

        return QImage();
    }

    HGDIOBJ oldObject = SelectObject(memoryDc, bitmap);

    const BOOL ok = BitBlt(memoryDc,
                           0,
                           0,
                           width,
                           height,
                           screenDc,
                           rect.x(),
                           rect.y(),
                           SRCCOPY | CAPTUREBLT);

    QImage image;

    if (ok)
        image = makeImageFromDibBits(bits, width, height, width * 4);

    SelectObject(memoryDc, oldObject);
    DeleteDC(memoryDc);
    DeleteObject(bitmap);
    ReleaseDC(nullptr, screenDc);

    if (!ok)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Visible window BitBlt fallback failed.");

        return QImage();
    }

    if (imageLooksEmpty(image))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Visible window BitBlt fallback produced an empty frame.");

        return QImage();
    }

    return image;
}

QImage captureWindowFallback(HWND hwnd,
                             const QString &wgcError,
                             QString *errorMessage)
{
    qWarning().noquote()
        << "[WindowCapture] WGC failed, trying PrintWindow fallback:"
        << wgcError;

    QString printWindowError;
    QImage image = captureWindowWithPrintWindow(hwnd, &printWindowError);

    if (!image.isNull())
    {
        qWarning().noquote()
            << "[WindowCapture] PrintWindow fallback succeeded.";

        return image;
    }

    qWarning().noquote()
        << "[WindowCapture] PrintWindow fallback failed:"
        << printWindowError;

    QString bitBltError;
    image = captureVisibleWindowRectWithBitBlt(hwnd, &bitBltError);

    if (!image.isNull())
    {
        qWarning().noquote()
            << "[WindowCapture] Visible window BitBlt fallback succeeded.";

        return image;
    }

    if (errorMessage)
    {
        *errorMessage = QStringLiteral(
            "Window capture failed.\n\n"
            "Windows Graphics Capture error:\n%1\n\n"
            "PrintWindow fallback error:\n%2\n\n"
            "Visible window fallback error:\n%3")
            .arg(wgcError,
                 printWindowError,
                 bitBltError);
    }

    return QImage();
}

// =====================================================================================================================
// Windows Graphics Capture helpers
// =====================================================================================================================

winrt::com_ptr<ID3D11Device> createD3D11Device(winrt::com_ptr<ID3D11DeviceContext> &context)
{
    constexpr UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    winrt::com_ptr<ID3D11Device> device;
    D3D_FEATURE_LEVEL selectedLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   flags,
                                   levels,
                                   ARRAYSIZE(levels),
                                   D3D11_SDK_VERSION,
                                   device.put(),
                                   &selectedLevel,
                                   context.put());

    if (FAILED(hr))
    {
        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_WARP,
                               nullptr,
                               flags,
                               levels,
                               ARRAYSIZE(levels),
                               D3D11_SDK_VERSION,
                               device.put(),
                               &selectedLevel,
                               context.put());
    }

    winrt::check_hresult(hr);

    if (!device || !context)
        winrt::throw_hresult(E_FAIL);

    return device;
}

IDirect3DDevice createDirect3DDevice(const winrt::com_ptr<ID3D11Device> &d3dDevice)
{
    if (!d3dDevice)
        return nullptr;

    winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice.as<IDXGIDevice>();

    if (!dxgiDevice)
        return nullptr;

    winrt::com_ptr<::IInspectable> inspectable;

    winrt::check_hresult(
        CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(),
                                             inspectable.put()));

    if (!inspectable)
        return nullptr;

    return inspectable.as<IDirect3DDevice>();
}

GraphicsCaptureItem createItemForWindow(HWND hwnd,
                                        QString *errorMessage)
{
    if (!hwnd || !IsWindow(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid target window handle.");

        return nullptr;
    }

    try
    {
        auto factory = winrt::get_activation_factory<GraphicsCaptureItem>();

        auto interop = factory.try_as<IGraphicsCaptureItemInterop>();

        if (!interop)
        {
            if (errorMessage)
            {
                *errorMessage =
                    QStringLiteral("IGraphicsCaptureItemInterop is not available. "
                                   "This usually happens on Windows 10 LTSC 1809 / build 17763. "
                                   "CreateForWindow requires Windows 10 1903 / build 18362 or newer.");
            }

            return nullptr;
        }

        GraphicsCaptureItem item{nullptr};

        const HRESULT hr = interop->CreateForWindow(hwnd,
                                                    winrt::guid_of<GraphicsCaptureItem>(),
                                                    winrt::put_abi(item));

        if (FAILED(hr))
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("CreateForWindow failed: %1")
                    .arg(hresultHex(hr));
            }

            return nullptr;
        }

        if (!item)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("CreateForWindow returned a null GraphicsCaptureItem.");

            return nullptr;
        }

        return item;
    }
    catch (const winrt::hresult_error &error)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("createItemForWindow exception: %1")
                .arg(hresultToString(error));
        }

        return nullptr;
    }
    catch (...)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unknown createItemForWindow exception.");

        return nullptr;
    }
}

QImage frameToImage(const Direct3D11CaptureFrame &frame,
                    const winrt::com_ptr<ID3D11Device> &device,
                    const winrt::com_ptr<ID3D11DeviceContext> &context)
{
    if (!frame || !device || !context)
        return QImage();

    const auto surface = frame.Surface();

    if (!surface)
        return QImage();

    auto surfaceAccess =
        surface.try_as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

    if (!surfaceAccess)
        return QImage();

    winrt::com_ptr<ID3D11Texture2D> texture;

    winrt::check_hresult(
        surfaceAccess->GetInterface(__uuidof(ID3D11Texture2D),
                                    texture.put_void()));

    if (!texture)
        return QImage();

    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);

    if (desc.Width == 0 || desc.Height == 0)
        return QImage();

    if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM
        && desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
    {
        qWarning() << "Unexpected WGC texture format:" << desc.Format;
    }

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.BindFlags = 0;
    stagingDesc.MiscFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage = D3D11_USAGE_STAGING;

    winrt::com_ptr<ID3D11Texture2D> staging;

    winrt::check_hresult(
        device->CreateTexture2D(&stagingDesc,
                                nullptr,
                                staging.put()));

    if (!staging)
        return QImage();

    context->CopyResource(staging.get(),
                          texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped = {};

    winrt::check_hresult(
        context->Map(staging.get(),
                     0,
                     D3D11_MAP_READ,
                     0,
                     &mapped));

    QImage image(static_cast<int>(desc.Width),
                 static_cast<int>(desc.Height),
                 QImage::Format_ARGB32);

    if (image.isNull())
    {
        context->Unmap(staging.get(), 0);
        return QImage();
    }

    for (UINT y = 0; y < desc.Height; ++y)
    {
        const auto *src =
            static_cast<const uchar *>(mapped.pData)
            + static_cast<size_t>(y) * mapped.RowPitch;

        uchar *dst = image.scanLine(static_cast<int>(y));

        std::memcpy(dst,
                    src,
                    static_cast<size_t>(desc.Width) * 4U);
    }

    context->Unmap(staging.get(), 0);

    return image;
}

QImage captureWindowWithWgc(HWND hwnd,
                            QString *errorMessage,
                            int timeoutMs)
{
    try
    {
        ensureWinRtInitialized();

        if (!GraphicsCaptureSession::IsSupported())
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Windows Graphics Capture is not supported on this system.");

            return QImage();
        }

        if (IsIconic(hwnd))
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("The target window is minimized. WGC cannot capture minimized windows.");

            return QImage();
        }

        winrt::com_ptr<ID3D11DeviceContext> d3dContext;
        const winrt::com_ptr<ID3D11Device> d3dDevice = createD3D11Device(d3dContext);

        if (!d3dDevice || !d3dContext)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Could not create the Direct3D 11 device.");

            return QImage();
        }

        const IDirect3DDevice device = createDirect3DDevice(d3dDevice);

        if (!device)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Could not create the WinRT Direct3D device.");

            return QImage();
        }

        QString createItemError;
        const GraphicsCaptureItem item = createItemForWindow(hwnd, &createItemError);

        if (!item)
        {
            if (errorMessage)
            {
                *errorMessage = createItemError.isEmpty()
                    ? QStringLiteral("Could not create the Windows Graphics Capture item for the target window.")
                    : createItemError;
            }

            return QImage();
        }

        const auto size = item.Size();

        if (size.Width <= 0 || size.Height <= 0)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("The target window has an invalid size.");

            return QImage();
        }

        Direct3D11CaptureFramePool framePool =
            Direct3D11CaptureFramePool::CreateFreeThreaded(device,
                                                           DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                                           1,
                                                           size);

        if (!framePool)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Could not create the Windows Graphics Capture frame pool.");

            return QImage();
        }

        GraphicsCaptureSession session = framePool.CreateCaptureSession(item);

        if (!session)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Could not create the Windows Graphics Capture session.");

            framePool.Close();
            return QImage();
        }

        try
        {
            session.IsCursorCaptureEnabled(false);
        }
        catch (...)
        {
            // Older Windows builds may not support this property. Not critical.
        }

        auto state = std::make_shared<CaptureState>();

        winrt::event_token frameArrivedToken{};
        bool frameArrivedRegistered = false;

        frameArrivedToken = framePool.FrameArrived(
            [state, d3dDevice, d3dContext](const Direct3D11CaptureFramePool &sender,
                                           const winrt::Windows::Foundation::IInspectable &)
            {
                QImage localImage;
                QString localError;

                try
                {
                    Direct3D11CaptureFrame frame = sender.TryGetNextFrame();

                    if (!frame)
                    {
                        localError = QStringLiteral("Windows Graphics Capture returned an empty frame.");
                    }
                    else
                    {
                        localImage = frameToImage(frame,
                                                  d3dDevice,
                                                  d3dContext);
                    }
                }
                catch (const winrt::hresult_error &error)
                {
                    localError = QStringLiteral("Windows Graphics Capture frame error: %1")
                        .arg(hresultToString(error));

                    qWarning() << localError;
                }
                catch (const std::exception &error)
                {
                    localError = QStringLiteral("Windows Graphics Capture frame std::exception: %1")
                        .arg(QString::fromLocal8Bit(error.what()));

                    qWarning() << localError;
                }
                catch (...)
                {
                    localError = QStringLiteral("Unknown Windows Graphics Capture frame error.");
                    qWarning() << localError;
                }

                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->image = localImage;
                    state->error = localError;
                    state->finished = true;
                }

                state->condition.notify_one();
            });

        frameArrivedRegistered = true;

        session.StartCapture();

        bool gotFrame = false;

        {
            std::unique_lock<std::mutex> lock(state->mutex);

            gotFrame = state->condition.wait_for(
                lock,
                std::chrono::milliseconds(timeoutMs),
                [state]()
                {
                    return state->finished;
                });
        }

        /*
         * Important:
         * Do not use winrt::auto_revoke here. Explicitly unregister the event
         * before closing the capture objects. Otherwise the event_revoker destructor
         * may throw during stack unwinding/cleanup and terminate the process.
         */
        if (frameArrivedRegistered)
        {
            try
            {
                framePool.FrameArrived(frameArrivedToken);
            }
            catch (const winrt::hresult_error &error)
            {
                qWarning() << "Could not unregister WGC FrameArrived event:"
                           << hresultToString(error);
            }
            catch (...)
            {
                qWarning() << "Could not unregister WGC FrameArrived event.";
            }
        }

        try
        {
            session.Close();
        }
        catch (...)
        {
            qWarning() << "Could not close WGC capture session.";
        }

        try
        {
            framePool.Close();
        }
        catch (...)
        {
            qWarning() << "Could not close WGC frame pool.";
        }

        QImage result;
        QString localError;

        {
            std::lock_guard<std::mutex> lock(state->mutex);
            result = state->image;
            localError = state->error;
        }

        if (!gotFrame)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Windows Graphics Capture did not produce a frame before timeout.");

            return QImage();
        }

        if (result.isNull())
        {
            if (errorMessage)
            {
                *errorMessage = localError.isEmpty()
                    ? QStringLiteral("Windows Graphics Capture returned an invalid frame.")
                    : localError;
            }

            return QImage();
        }

        if (imageLooksEmpty(result))
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("Windows Graphics Capture produced an empty frame.");

            return QImage();
        }

        return result;
    }
    catch (const winrt::hresult_error &error)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Windows Graphics Capture failed: %1")
                .arg(hresultToString(error));
        }

        return QImage();
    }
    catch (const std::exception &error)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Windows Graphics Capture std::exception: %1")
                .arg(QString::fromLocal8Bit(error.what()));
        }

        return QImage();
    }
    catch (...)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unknown Windows Graphics Capture failure.");

        return QImage();
    }
}

} // namespace

#endif // Q_OS_WIN

namespace sbpshots::capture {

bool WindowCaptureWgc::isSupported()
{
#ifdef Q_OS_WIN
    /*
     * This class supports two window capture paths:
     *
     * 1. Windows Graphics Capture, available on modern Windows versions.
     * 2. Win32 fallbacks, such as PrintWindow / BitBlt, required for
     *    Windows 10 LTSC 1809 where IGraphicsCaptureItemInterop is missing.
     *
     * Therefore, on Windows this function returns true even if WGC itself is
     * not available. The actual capture method is selected inside captureWindow().
     */
    return true;
#else
    return false;
#endif
}

bool WindowCaptureWgc::isWgcSupported()
{
#ifdef Q_OS_WIN
    try
    {
        ensureWinRtInitialized();
        return GraphicsCaptureSession::IsSupported();
    }
    catch (const winrt::hresult_error &error)
    {
        qWarning() << "Windows Graphics Capture support check failed:"
                   << hresultToString(error);
        return false;
    }
    catch (const std::exception &error)
    {
        qWarning() << "Windows Graphics Capture support check failed:"
                   << error.what();
        return false;
    }
    catch (...)
    {
        qWarning() << "Windows Graphics Capture support check failed with unknown error.";
        return false;
    }
#else
    return false;
#endif
}

QImage WindowCaptureWgc::captureWindow(quintptr windowId,
                                       QString *errorMessage,
                                       int timeoutMs)
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(windowId);

    if (!hwnd || !IsWindow(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window handle is not valid.");

        return QImage();
    }

    if (!IsWindowVisible(hwnd))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The target window is not visible.");

        return QImage();
    }

    QString wgcError;
    QImage image = captureWindowWithWgc(hwnd,
                                        &wgcError,
                                        timeoutMs);

    if (!image.isNull())
        return image;

    return captureWindowFallback(hwnd,
                                 wgcError.isEmpty()
                                     ? QStringLiteral("Windows Graphics Capture failed without a detailed error.")
                                     : wgcError,
                                 errorMessage);
#else
    Q_UNUSED(windowId)
    Q_UNUSED(timeoutMs)

    if (errorMessage)
        *errorMessage = QStringLiteral("Window capture is only available on Windows.");

    return QImage();
#endif
}

} // namespace sbpshots::capture
