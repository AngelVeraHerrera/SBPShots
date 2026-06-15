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
 * @file    campaign_sequence.cpp
 * @brief   Oceanographic campaign sequence tracking (survey lines and transit sections).
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// TOPASSHOTS INCLUDES
#include "TopasShots/core/campaign_sequence.h"
#include "TopasShots/core/path_tokens.h"

// QT INCLUDES
#include <QChar>

// =====================================================================================================================

namespace tshots::core {

void CampaignSequence::normalize()
{
    linePrefix = PathTokens::requiredPathToken(linePrefix).toUpper();
    transitionPrefix = PathTokens::requiredPathToken(transitionPrefix).toUpper();
    suffix = PathTokens::requiredPathToken(suffix).toUpper();

    if (linePrefix.isEmpty())
        linePrefix = QStringLiteral("L");

    if (transitionPrefix.isEmpty())
        transitionPrefix = QStringLiteral("TL");

    if (number < 1)
        number = 1;
}

QString CampaignSequence::currentNumberToken() const
{
    return QString::number(number).rightJustified(3, QLatin1Char('0'));
}

QString CampaignSequence::currentSuffixToken() const
{
    return PathTokens::requiredPathToken(suffix).toUpper();
}

QString CampaignSequence::currentLinePrefixToken() const
{
    return PathTokens::requiredPathToken(linePrefix).toUpper();
}

QString CampaignSequence::currentTransitionPrefixToken() const
{
    return PathTokens::requiredPathToken(transitionPrefix).toUpper();
}

QString CampaignSequence::lineId() const
{
    return currentLinePrefixToken() + currentNumberToken() + currentSuffixToken();
}

QString CampaignSequence::transitionId() const
{
    return currentTransitionPrefixToken() + currentNumberToken() + currentSuffixToken();
}

QString CampaignSequence::currentSectionId() const
{
    return currentKind == SectionKind::Transition ? transitionId() : lineId();
}

QString CampaignSequence::currentSectionLabel() const
{
    return currentKind == SectionKind::Transition ? QStringLiteral("Transition")
                                                  : QStringLiteral("Line");
}

void CampaignSequence::goNext()
{
    normalize();

    if (currentKind == SectionKind::Transition)
    {
        currentKind = SectionKind::Line;
        return;
    }

    ++number;
    currentKind = SectionKind::Transition;
}

void CampaignSequence::goPrevious()
{
    normalize();

    if (currentKind == SectionKind::Line)
    {
        currentKind = SectionKind::Transition;
        return;
    }

    if (number > 1)
    {
        --number;
        currentKind = SectionKind::Line;
    }
}

} // namespace lst::core
