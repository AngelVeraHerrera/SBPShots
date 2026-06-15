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
 * @file    shot_types.h
 * @brief   Enumerations for shot source origin and image frame kind.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// =====================================================================================================================

namespace sbpshots::core
{

// =====================================================================================================================

/**
 * @brief Identifies how a screenshot was triggered.
 */
enum class ShotSource
{
    Manual,      ///< Triggered by the user via the manual capture button.
    Auto,        ///< Triggered by the automatic periodic capture timer.
    Independent  ///< Ad-hoc capture saved to a user-chosen folder.
};

// =====================================================================================================================

/**
 * @brief Identifies the image frame type within a captured shot pair.
 */
enum class FrameKind
{
    Full,   ///< Full-frame capture of the target.
    Crop    ///< ROI-cropped region extracted from the full frame.
};

// =====================================================================================================================

} // namespace sbpshots::core
