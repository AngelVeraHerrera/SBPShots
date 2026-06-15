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
 * @file    screenshot_service.cpp
 * @brief   High-level screenshot capture service with full-frame and ROI crop support.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/capture/screenshot_service.h"
#include "SBPShots/capture/capture_target_enumerator.h"
#include "SBPShots/capture/window_capture_wgc.h"

// QT INCLUDES
#include <QGuiApplication>
#include <QScreen>

// =====================================================================================================================

namespace sbpshots::capture {

QScreen *ScreenshotService::screenForTarget(const CaptureTarget &target)
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    if (target.type == CaptureTargetType::Screen)
    {
        if (target.screenIndex >= 0 && target.screenIndex < screens.size())
            return screens.at(target.screenIndex);

        return QGuiApplication::primaryScreen();
    }

    const QRect rect = CaptureTargetEnumerator::windowRect(target.windowId);

    if (rect.isValid() && !rect.isEmpty())
    {
        if (QScreen *screen = QGuiApplication::screenAt(rect.center()))
            return screen;
    }

    return QGuiApplication::primaryScreen();
}

QPixmap ScreenshotService::grabTargetFull(const CaptureTarget &target,
                                          QScreen **usedScreen,
                                          QString *errorMessage)
{
    QScreen *screen = screenForTarget(target);

    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (usedScreen)
        *usedScreen = screen;

    if (!screen)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("No screen is available for capture.");
        return QPixmap();
    }

    if (target.type == CaptureTargetType::Window)
    {
        QString wgcError;
        const QImage image = WindowCaptureWgc::captureWindow(target.windowId, &wgcError);

        if (!image.isNull())
            return QPixmap::fromImage(image);

        if (errorMessage)
            *errorMessage = wgcError.isEmpty()
                ? QStringLiteral("The selected target window could not be captured.")
                : wgcError;

        return QPixmap();
    }

    return screen->grabWindow(0);
}

QPixmap ScreenshotService::cropToRoi(const QPixmap &source,
                                     const QRect &roi,
                                     const CaptureTarget &target,
                                     QScreen *screen,
                                     QString *errorMessage)
{
    if (source.isNull())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The source capture is empty.");
        return QPixmap();
    }

    if (!screen)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("No screen is available for ROI conversion.");
        return QPixmap();
    }

    if (!roi.isValid() || roi.isEmpty() || roi.width() <= 5 || roi.height() <= 5)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The ROI is not configured or is invalid.");
        return QPixmap();
    }

    QRect relativeLogicalRoi;

    if (target.type == CaptureTargetType::Window)
    {
        relativeLogicalRoi = roi;
    }
    else
    {
        const QRect screenGeometry = screen->geometry();
        const QRect roiOnScreen = roi.intersected(screenGeometry);

        if (!roiOnScreen.isValid() || roiOnScreen.isEmpty())
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("The ROI is outside the selected screen.");
            return QPixmap();
        }

        relativeLogicalRoi = roiOnScreen.translated(-screenGeometry.topLeft());
    }

    const qreal dpr = source.devicePixelRatio();
    const QRect pixelRoi(qRound(relativeLogicalRoi.x() * dpr),
                         qRound(relativeLogicalRoi.y() * dpr),
                         qRound(relativeLogicalRoi.width() * dpr),
                         qRound(relativeLogicalRoi.height() * dpr));

    const QRect pixmapRect(QPoint(0, 0), source.size());
    const QRect validPixelRoi = pixelRoi.intersected(pixmapRect);

    if (!validPixelRoi.isValid() || validPixelRoi.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The ROI crop is outside the captured image.");
        return QPixmap();
    }

    QPixmap cropped = source.copy(validPixelRoi);
    cropped.setDevicePixelRatio(dpr);
    return cropped;
}

ScreenshotResult ScreenshotService::capture(const CaptureTarget &target,
                                            const QRect &roi)
{
    ScreenshotResult result;
    result.full = grabTargetFull(target, &result.screen, &result.errorMessage);

    if (result.full.isNull())
        return result;

    result.crop = cropToRoi(result.full,
                            roi,
                            target,
                            result.screen,
                            &result.errorMessage);

    return result;
}

} // namespace sbpshots::capture
