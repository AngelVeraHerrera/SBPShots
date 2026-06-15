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
 * @file    capture_target_enumerator.h
 * @brief   Enumerates available screen and window capture targets.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// SBPSHOTS INCLUDES
#include "SBPShots/capture/capture_target.h"

// QT INCLUDES
#include <QList>

// =====================================================================================================================

namespace sbpshots::capture
{

// =====================================================================================================================

/**
 * @brief Provides static helpers for discovering and querying capture targets.
 *
 * All methods are static; the class is not intended to be instantiated.
 */
class CaptureTargetEnumerator final
{
public:

    /**
     * @brief Enumerates all available screens and visible application windows.
     * @param selfWindowId Native handle of the caller's own window to exclude from results.
     * @return List of all available capture targets.
     */
    static QList<CaptureTarget> enumerateTargets(quintptr selfWindowId = 0);

    /**
     * @brief Tries to locate a window whose title matches the given string.
     *
     * Performs an exact match first, then a case-insensitive partial match.
     *
     * @param title  Window title to search for.
     * @param target Output capture target, populated on success.
     * @return True if a matching window was found.
     */
    static bool findWindowByTitle(const QString &title, CaptureTarget *target);

    /**
     * @brief Returns true if the window identified by @p windowId is currently visible.
     * @param windowId Native window handle stored as quintptr.
     */
    static bool isWindowAvailable(quintptr windowId);

    /**
     * @brief Returns the current geometry of a window in global screen coordinates.
     * @param windowId Native window handle stored as quintptr.
     * @return Window geometry, or an invalid QRect if the window is not accessible.
     */
    static QRect windowRect(quintptr windowId);

    /**
     * @brief Returns a CaptureTarget representing the primary screen.
     */
    static CaptureTarget primaryScreenTarget();
};

// =====================================================================================================================

} // namespace sbpshots::capture
