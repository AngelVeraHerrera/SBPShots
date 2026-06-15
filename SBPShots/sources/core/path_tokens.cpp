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
 * @file    path_tokens.cpp
 * @brief   Utilities for sanitizing strings into safe file-system path tokens.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/core/path_tokens.h"

// QT INCLUDES
#include <QRegularExpression>

// =====================================================================================================================

namespace sbpshots::core
{

QString PathTokens::requiredPathToken(const QString &text)
{
    static const QRegularExpression invalidCharsRegex(R"([<>:"/\\|?*\x00-\x1F])");
    static const QRegularExpression spacesRegex(R"(\s+)");
    static const QRegularExpression underscoresRegex(R"(_+)");

    QString token = text.trimmed();
    token.replace(invalidCharsRegex, "_");
    token.replace(spacesRegex, "_");
    token.replace(underscoresRegex, "_");

    while (token.startsWith('_'))
        token.remove(0, 1);

    while (token.endsWith('_'))
        token.chop(1);

    return token;
}

QString PathTokens::safePathToken(const QString &text)
{
    QString token = requiredPathToken(text);

    if (token.isEmpty())
        token = QStringLiteral("unnamed");

    return token;
}

QString PathTokens::markerToken(const QString &text)
{
    static const QRegularExpression invalidCharsRegex(R"([<>:"/\\|?*\x00-\x1F])");
    static const QRegularExpression spacesRegex(R"(\s+)");
    static const QRegularExpression hyphensRegex(R"(-+)");

    QString token = text.trimmed();
    token.replace(invalidCharsRegex, "_");
    token.replace(spacesRegex, "-");
    token.replace(hyphensRegex, "-");

    while (token.startsWith('_') || token.startsWith('-'))
        token.remove(0, 1);

    while (token.endsWith('_') || token.endsWith('-'))
        token.chop(1);

    return token;
}

} // namespace sbpshots::core
