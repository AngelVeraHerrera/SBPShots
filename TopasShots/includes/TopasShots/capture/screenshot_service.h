/***********************************************************************************************************************
 *   TopasShots - Screenshot automation tool for TOPAS sensor monitoring during oceanographic surveys.                 *
 *                                                                                                                     *
 *   Copyright (C) 2022-2026 Angel Vera Herrera <avera@roa.es>                                                         *
 *                           Real Instituto y Observatorio de la Armada (ROA)                                          *
 *                                                                                                                     *
 *   This file is part of TopasShots.                                                                                  *
 *                                                                                                                     *
 *   TopasShots is free software: you can redistribute it and/or modify it under the terms of the GNU General          *
 *   Public License as published by the Free Software Foundation, either version 3 of the License, or (at your         *
 *   option) any later version.                                                                                        *
 *                                                                                                                     *
 *   TopasShots is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the          *
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License       *
 *   for more details.                                                                                                 *
 *                                                                                                                     *
 *   You should have received a copy of the GNU General Public License along with TopasShots.                          *
 *   If not, see <http://www.gnu.org/licenses/>.                                                                       *
 **********************************************************************************************************************/

/** ********************************************************************************************************************
 * @file    screenshot_service.h
 * @brief   High-level screenshot capture service with full-frame and ROI crop support.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// TOPASSHOTS INCLUDES
#include "TopasShots/capture/capture_target.h"

// QT INCLUDES
#include <QPixmap>
#include <QRect>
#include <QString>

class QScreen;

// =====================================================================================================================

namespace tshots::capture
{

// =====================================================================================================================

/**
 * @brief Holds the result of a complete capture operation.
 */
struct ScreenshotResult
{
    QPixmap full;                ///< Full-frame capture of the target.
    QPixmap crop;                ///< ROI-cropped region extracted from the full frame.
    QScreen *screen = nullptr;   ///< Screen used for the capture.
    QString errorMessage;        ///< Non-empty when either capture step failed.
};

// =====================================================================================================================

/**
 * @brief Provides static methods for capturing screens and application windows.
 *
 * Uses WindowCaptureWgc for window targets and Qt screen grabbing for screen targets.
 * All methods are static; the class is not intended to be instantiated.
 */
class ScreenshotService final
{
public:

    /**
     * @brief Resolves the QScreen best associated with the given capture target.
     * @param target Capture target to resolve.
     * @return Associated QScreen, or the primary screen if resolution fails.
     */
    static QScreen *screenForTarget(const CaptureTarget &target);

    /**
     * @brief Grabs the full frame of a capture target.
     * @param target       Capture target (screen or window).
     * @param usedScreen   Output pointer to the screen used; may be null.
     * @param errorMessage Optional error description on failure.
     * @return Full-frame pixmap, or a null QPixmap on failure.
     */
    static QPixmap grabTargetFull(const CaptureTarget &target,
                                  QScreen **usedScreen,
                                  QString *errorMessage);

    /**
     * @brief Crops a full-frame pixmap to the configured ROI.
     * @param source        Source full-frame pixmap.
     * @param roi           Region of interest in global (or window-relative) coordinates.
     * @param target        Capture target used to interpret the ROI coordinate space.
     * @param screen        Screen associated with the capture.
     * @param errorMessage  Optional error description on failure.
     * @return Cropped pixmap, or a null QPixmap on failure.
     */
    static QPixmap cropToRoi(const QPixmap &source,
                             const QRect &roi,
                             const CaptureTarget &target,
                             QScreen *screen,
                             QString *errorMessage);

    /**
     * @brief Performs a complete capture: grabs the full frame and extracts the ROI crop.
     * @param target Capture target.
     * @param roi    Region of interest.
     * @return ScreenshotResult containing both images and any error message.
     */
    static ScreenshotResult capture(const CaptureTarget &target, const QRect &roi);
};

// =====================================================================================================================

} // namespace lst::capture
