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
 * @file    capture_target_enumerator.cpp
 * @brief   Enumerates available screen and window capture targets.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/capture/capture_target_enumerator.h"

// QT INCLUDES
#include <QGuiApplication>
#include <QScreen>
#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

// =====================================================================================================================

namespace {

#ifdef Q_OS_WIN

struct NativeWindowInfo
{
    quintptr id = 0;
    QString title;
    QRect rect;
};

BOOL CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam)
{
    auto *windows = reinterpret_cast<QVector<NativeWindowInfo> *>(lParam);

    if (!windows || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd))
        return TRUE;

    wchar_t titleBuffer[512] = {};
    const int titleLength = GetWindowTextW(hwnd, titleBuffer, 512);

    if (titleLength <= 0)
        return TRUE;

    const QString title = QString::fromWCharArray(titleBuffer, titleLength).trimmed();

    if (title.isEmpty())
        return TRUE;

    RECT nativeRect = {};
    if (!GetWindowRect(hwnd, &nativeRect))
        return TRUE;

    const int width = nativeRect.right - nativeRect.left;
    const int height = nativeRect.bottom - nativeRect.top;

    if (width < 50 || height < 50)
        return TRUE;

    windows->append(NativeWindowInfo{
        reinterpret_cast<quintptr>(hwnd),
        title,
        QRect(nativeRect.left, nativeRect.top, width, height)
    });

    return TRUE;
}

QVector<NativeWindowInfo> enumerateNativeWindows()
{
    QVector<NativeWindowInfo> windows;
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

#endif

} // namespace

// =====================================================================================================================

namespace sbpshots::capture {

QList<CaptureTarget> CaptureTargetEnumerator::enumerateTargets(quintptr selfWindowId)
{
    QList<CaptureTarget> targets;

    const QList<QScreen *> screens = QGuiApplication::screens();

    for (int i = 0; i < screens.size(); ++i)
    {
        const QScreen *screen = screens.at(i);
        if (!screen)
            continue;

        const QRect geometry = screen->geometry();
        CaptureTarget target;
        target.type = CaptureTargetType::Screen;
        target.screenIndex = i;
        target.windowId = 0;
        target.persistentName = screen->name();
        target.geometry = geometry;
        target.displayName = QStringLiteral("Screen %1: %2x%3 @ %4,%5%6")
            .arg(i + 1)
            .arg(geometry.width())
            .arg(geometry.height())
            .arg(geometry.x())
            .arg(geometry.y())
            .arg(screen == QGuiApplication::primaryScreen() ? QStringLiteral(" [Primary]") : QString());

        targets.append(target);
    }

#ifdef Q_OS_WIN
    const QVector<NativeWindowInfo> windows = enumerateNativeWindows();

    for (const NativeWindowInfo &window : windows)
    {
        if (selfWindowId != 0 && window.id == selfWindowId)
            continue;

        CaptureTarget target;
        target.type = CaptureTargetType::Window;
        target.screenIndex = -1;
        target.windowId = window.id;
        target.persistentName = window.title;
        target.geometry = window.rect;
        target.displayName = QStringLiteral("Window: %1 [%2x%3 @ %4,%5]")
            .arg(window.title)
            .arg(window.rect.width())
            .arg(window.rect.height())
            .arg(window.rect.x())
            .arg(window.rect.y());

        targets.append(target);
    }
#else
    Q_UNUSED(selfWindowId)
#endif

    return targets;
}

CaptureTarget CaptureTargetEnumerator::primaryScreenTarget()
{
    CaptureTarget target;
    target.type = CaptureTargetType::Screen;
    target.screenIndex = 0;
    target.windowId = 0;

    QScreen *primary = QGuiApplication::primaryScreen();
    const QList<QScreen *> screens = QGuiApplication::screens();

    if (primary)
    {
        const int index = qMax(0, screens.indexOf(primary));
        const QRect geometry = primary->geometry();
        target.screenIndex = index;
        target.geometry = geometry;
        target.persistentName = primary->name();
        target.displayName = QStringLiteral("Screen %1: %2x%3 @ %4,%5 [Primary]")
            .arg(index + 1)
            .arg(geometry.width())
            .arg(geometry.height())
            .arg(geometry.x())
            .arg(geometry.y());
    }
    else
    {
        target.displayName = QStringLiteral("Primary screen");
    }

    return target;
}

bool CaptureTargetEnumerator::findWindowByTitle(const QString &title, CaptureTarget *target)
{
#ifdef Q_OS_WIN
    if (title.trimmed().isEmpty())
        return false;

    const QVector<NativeWindowInfo> windows = enumerateNativeWindows();

    auto assign = [target](const NativeWindowInfo &window) {
        if (!target)
            return;
        target->type = CaptureTargetType::Window;
        target->screenIndex = -1;
        target->windowId = window.id;
        target->persistentName = window.title;
        target->displayName = QStringLiteral("Window: %1").arg(window.title);
        target->geometry = window.rect;
    };

    for (const NativeWindowInfo &window : windows)
    {
        if (window.title == title)
        {
            assign(window);
            return true;
        }
    }

    for (const NativeWindowInfo &window : windows)
    {
        if (window.title.contains(title, Qt::CaseInsensitive)
            || title.contains(window.title, Qt::CaseInsensitive))
        {
            assign(window);
            return true;
        }
    }
#else
    Q_UNUSED(title)
    Q_UNUSED(target)
#endif

    return false;
}

bool CaptureTargetEnumerator::isWindowAvailable(quintptr windowId)
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(windowId);
    return hwnd && IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd);
#else
    Q_UNUSED(windowId)
    return false;
#endif
}

QRect CaptureTargetEnumerator::windowRect(quintptr windowId)
{
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(windowId);

    if (!hwnd || !IsWindow(hwnd))
        return QRect();

    RECT nativeRect = {};
    const HRESULT result = DwmGetWindowAttribute(hwnd,
                                                 DWMWA_EXTENDED_FRAME_BOUNDS,
                                                 &nativeRect,
                                                 sizeof(nativeRect));

    if (SUCCEEDED(result))
    {
        const int width = nativeRect.right - nativeRect.left;
        const int height = nativeRect.bottom - nativeRect.top;
        if (width > 0 && height > 0)
            return QRect(nativeRect.left, nativeRect.top, width, height);
    }

    if (!GetWindowRect(hwnd, &nativeRect))
        return QRect();

    return QRect(nativeRect.left,
                 nativeRect.top,
                 nativeRect.right - nativeRect.left,
                 nativeRect.bottom - nativeRect.top);
#else
    Q_UNUSED(windowId)
    return QRect();
#endif
}

} // namespace sbpshots::capture
