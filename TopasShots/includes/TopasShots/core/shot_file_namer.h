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
 * @file    shot_file_namer.h
 * @brief   Generates context-aware, timestamped filenames for captured screenshot pairs.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// TOPASSHOTS INCLUDES
#include "TopasShots/core/shot_types.h"

// QT INCLUDES
#include <QString>

// =====================================================================================================================

namespace tshots::core
{

// =====================================================================================================================

/**
 * @brief Context used to derive a screenshot filename.
 */
struct ShotNamingContext
{
    QString campaign;                         ///< Campaign identifier token.
    QString sectionId;                        ///< Section identifier token (e.g. "L001A").
    ShotSource source = ShotSource::Manual;   ///< Shot source origin.
    FrameKind frame = FrameKind::Full;        ///< Frame kind (full or crop).
    QString marker;                           ///< Optional user marker appended to the filename.
};

// =====================================================================================================================

/**
 * @brief Generates deterministic, timestamped filenames for screenshot files.
 *
 * The filename format is:
 *   @c {campaign}_{sectionId}_{source}_{frame}_{timestamp}[_{marker}].JPG
 *
 * All methods are static; the class is not intended to be instantiated.
 */
class ShotFileNamer final
{
public:

    /**
     * @brief Returns the filename token for the given shot source.
     * @param source Shot source.
     * @return Token string ("manual", "auto", or "independent").
     */
    static QString sourceToken(ShotSource source);

    /**
     * @brief Returns the filename token for the given frame kind.
     * @param frame Frame kind.
     * @return Token string ("full" or "crop").
     */
    static QString frameToken(FrameKind frame);

    /**
     * @brief Constructs a complete filename from the given naming context.
     * @param context Naming context with all required fields.
     * @return Complete filename including timestamp and .JPG extension.
     */
    static QString fileName(const ShotNamingContext &context);
};

// =====================================================================================================================

} // namespace lst::core
