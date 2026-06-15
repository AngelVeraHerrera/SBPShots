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
 * @file    window_capture_wgc.h
 * @brief   Window capture backend using Windows Graphics Capture with Win32 fallbacks.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QImage>
#include <QString>

// =====================================================================================================================

namespace tshots::capture
{

// =====================================================================================================================

/**
 * @brief Captures application windows using Windows Graphics Capture (WGC).
 *
 * The primary path uses the Windows Graphics Capture API, available on Windows 10 1903
 * (build 18362) and later. On older builds such as Windows 10 LTSC 1809 the class
 * transparently falls back to Win32 capture methods (PrintWindow, then BitBlt).
 *
 * All methods are static; the class is not intended to be instantiated.
 */
class WindowCaptureWgc final
{
public:

    /**
     * @brief Returns true when window capture is available on the current platform.
     *
     * On Windows this always returns true because the class can use either WGC or
     * Win32 fallbacks. Use isWgcSupported() to check whether WGC itself is available.
     */
    static bool isSupported();

    /**
     * @brief Returns true when the Windows Graphics Capture API is available.
     *
     * WGC requires Windows 10 1903 (build 18362) or later.
     */
    static bool isWgcSupported();

    /**
     * @brief Captures the contents of a target window.
     *
     * Capture order on Windows:
     *   1. Windows Graphics Capture (preferred).
     *   2. PrintWindow fallback.
     *   3. Visible-rectangle BitBlt fallback.
     *
     * @param windowId     Native HWND stored as quintptr.
     * @param errorMessage Optional; receives a detailed error description on failure.
     * @param timeoutMs    Timeout in milliseconds for the WGC frame wait.
     * @return Captured image, or a null QImage on failure.
     */
    static QImage captureWindow(quintptr windowId,
                                QString *errorMessage = nullptr,
                                int timeoutMs = 1500);
};

// =====================================================================================================================

} // namespace lst::capture
