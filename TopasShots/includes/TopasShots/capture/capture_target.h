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
 * @file    capture_target.h
 * @brief   Data types representing a capture target (screen or application window).
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QRect>
#include <QString>

// =====================================================================================================================

namespace tshots::capture
{

// =====================================================================================================================

/**
 * @brief Identifies the kind of capture target.
 */
enum class CaptureTargetType
{
    Screen,   ///< A physical or virtual screen.
    Window    ///< An application window.
};

// =====================================================================================================================

/**
 * @brief Represents a single capture source, either a screen or an application window.
 *
 * Stores all information needed to identify, display, and restore a capture target
 * across application sessions.
 */
struct CaptureTarget
{
    CaptureTargetType type = CaptureTargetType::Screen;
    QString displayName;         ///< Human-readable label shown in the UI.
    QString persistentName;      ///< Stable identifier used to restore the target across sessions.
    int screenIndex = 0;         ///< Screen index (valid when type is Screen).
    quintptr windowId = 0;       ///< Native window handle (valid when type is Window).
    QRect geometry;              ///< Last known geometry in global screen coordinates.

    /** @brief Returns true if this target is an application window. */
    bool isWindow() const { return type == CaptureTargetType::Window; }

    /** @brief Returns true if this target is a screen. */
    bool isScreen() const { return type == CaptureTargetType::Screen; }
};

// =====================================================================================================================

} // namespace lst::capture
