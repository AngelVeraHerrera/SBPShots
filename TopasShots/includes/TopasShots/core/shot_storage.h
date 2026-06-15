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
 * @file    shot_storage.h
 * @brief   Screenshot file storage and campaign directory management.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// TOPASSHOTS INCLUDES
#include "TopasShots/core/shot_file_namer.h"

// QT INCLUDES
#include <QPixmap>
#include <QString>

// =====================================================================================================================

namespace tshots::core
{

// =====================================================================================================================

/**
 * @brief Holds the file-system paths of a saved full/crop screenshot pair.
 */
struct SavedShotPaths
{
    QString fullPath;   ///< Absolute path to the saved full-frame image.
    QString cropPath;   ///< Absolute path to the saved ROI-crop image.
};

// =====================================================================================================================

/**
 * @brief Saves screenshot pairs to an organised campaign directory structure.
 *
 * Campaign shots are stored under:
 *   @c {basePath}/{campaign}/{sectionId}/{source}/{frame}/{filename}
 *
 * All methods are static; the class is not intended to be instantiated.
 */
class ShotStorage final
{
public:

    /**
     * @brief Computes the directory path for a given shot context.
     * @param basePath   Root storage directory.
     * @param campaign   Campaign identifier token.
     * @param sectionId  Section identifier token.
     * @param source     Shot source origin.
     * @param frame      Frame kind (full or crop).
     * @return Absolute directory path.
     */
    static QString shotDirectory(const QString &basePath,
                                 const QString &campaign,
                                 const QString &sectionId,
                                 ShotSource source,
                                 FrameKind frame);

    /**
     * @brief Saves a full/crop screenshot pair under the campaign directory structure.
     * @param basePath      Root storage directory.
     * @param campaign      Campaign identifier token.
     * @param sectionId     Section identifier token.
     * @param source        Shot source origin.
     * @param marker        Optional filename marker string.
     * @param full          Full-frame pixmap to save.
     * @param crop          ROI-crop pixmap to save.
     * @param paths         Optional; receives the paths of the saved files.
     * @param errorMessage  Optional; receives a description on failure.
     * @return True on success.
     */
    static bool saveShotPair(const QString &basePath,
                             const QString &campaign,
                             const QString &sectionId,
                             ShotSource source,
                             const QString &marker,
                             const QPixmap &full,
                             const QPixmap &crop,
                             SavedShotPaths *paths,
                             QString *errorMessage);

    /**
     * @brief Saves a full/crop screenshot pair to an arbitrary output folder.
     * @param folder        Destination directory (will be created if needed).
     * @param marker        Optional filename marker string.
     * @param full          Full-frame pixmap to save.
     * @param crop          ROI-crop pixmap to save.
     * @param paths         Optional; receives the paths of the saved files.
     * @param errorMessage  Optional; receives a description on failure.
     * @return True on success.
     */
    static bool saveIndependentShotPair(const QString &folder,
                                        const QString &marker,
                                        const QPixmap &full,
                                        const QPixmap &crop,
                                        SavedShotPaths *paths,
                                        QString *errorMessage);
};

// =====================================================================================================================

} // namespace lst::core
