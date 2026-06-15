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
 * @file    app_settings.h
 * @brief   Application settings data structure and INI-based persistence.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// SBPSHOTS INCLUDES
#include "SBPShots/capture/capture_target.h"
#include "SBPShots/core/campaign_sequence.h"

// QT INCLUDES
#include <QRect>
#include <QString>

// =====================================================================================================================

namespace sbpshots::core
{

// =====================================================================================================================

/**
 * @brief Central settings data structure holding all persistent application state.
 */
struct ApplicationSettings
{
    QString basePath;                        ///< Root directory for saving screenshots.
    QString campaign;                        ///< Campaign identifier used as a directory name.
    CampaignSequence sequence;               ///< Current oceanographic survey sequence state.
    QRect roi;                               ///< Region of interest in global screen coordinates.
    sbpshots::capture::CaptureTarget captureTarget; ///< Currently selected capture source.
    bool alwaysOnTop = false;                ///< Whether the application window stays on top.
};

// =====================================================================================================================

/**
 * @brief Loads and saves ApplicationSettings to a per-application INI file.
 *
 * The INI file is located at @c {applicationDirPath}/config.ini.
 * All methods are static; the class is not intended to be instantiated.
 */
class AppSettingsStore final
{
public:

    /**
     * @brief Reads settings from the INI file and returns them.
     *
     * Missing keys fall back to default values. If the stored capture target
     * can no longer be found the primary screen is used instead.
     *
     * @return Loaded ApplicationSettings.
     */
    static ApplicationSettings load();

    /**
     * @brief Writes settings to the INI file.
     * @param settings Settings to persist.
     */
    static void save(const ApplicationSettings &settings);
};

// =====================================================================================================================

} // namespace sbpshots::core
