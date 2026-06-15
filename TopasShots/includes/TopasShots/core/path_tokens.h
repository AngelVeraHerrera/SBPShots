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
 * @file    path_tokens.h
 * @brief   Utilities for sanitizing strings into safe file-system path tokens.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QString>

// =====================================================================================================================

namespace tshots::core
{

// =====================================================================================================================

/**
 * @brief Provides static helpers for converting arbitrary strings into file-system-safe tokens.
 *
 * Invalid characters, spaces, and redundant underscores are removed or replaced.
 * All methods are static; the class is not intended to be instantiated.
 */
class PathTokens final
{
public:

    /**
     * @brief Strips or replaces characters that are invalid in file or directory names.
     *
     * Spaces are converted to underscores. Multiple consecutive underscores are collapsed.
     * Leading and trailing underscores are removed. Returns an empty string if the input
     * contains only invalid characters.
     *
     * @param text Input string.
     * @return Sanitized token, possibly empty.
     */
    static QString requiredPathToken(const QString &text);

    /**
     * @brief Same as requiredPathToken(), but returns @c "unnamed" when the result would be empty.
     * @param text Input string.
     * @return Sanitized token, never empty.
     */
    static QString safePathToken(const QString &text);

    /**
     * @brief Sanitizes a user-provided marker string for use in filenames.
     *
     * Similar to requiredPathToken() but preserves hyphens (replacing spaces with hyphens
     * rather than underscores).
     *
     * @param text Input string.
     * @return Sanitized marker token, possibly empty.
     */
    static QString markerToken(const QString &text);
};

// =====================================================================================================================

} // namespace lst::core
