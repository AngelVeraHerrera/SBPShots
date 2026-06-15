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
 * @file    campaign_sequence.h
 * @brief   Oceanographic campaign sequence tracking (survey lines and transit sections).
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QString>

// =====================================================================================================================

namespace sbpshots::core
{

// =====================================================================================================================

/**
 * @brief Distinguishes between a survey line section and a transit section.
 */
enum class SectionKind
{
    Transition,   ///< Transit between survey lines.
    Line          ///< Active survey line.
};

// =====================================================================================================================

/**
 * @brief Tracks the current position within an oceanographic survey campaign sequence.
 *
 * A campaign alternates between Transition and Line sections. The sequence advances
 * from the first transition (TL001) to the first line (L001), then to the second
 * transition (TL002), and so on. Each section can also carry an optional suffix
 * (e.g. A, B) to distinguish repeated passes.
 */
class CampaignSequence final
{
public:

    /** @brief Sanitizes prefixes and clamps the line number to a valid value. */
    void normalize();

    /** @brief Returns the current line number as a zero-padded three-digit string. */
    QString currentNumberToken() const;

    /** @brief Returns the sanitized, upper-cased suffix token (empty if no suffix). */
    QString currentSuffixToken() const;

    /** @brief Returns the sanitized, upper-cased line prefix token. */
    QString currentLinePrefixToken() const;

    /** @brief Returns the sanitized, upper-cased transition prefix token. */
    QString currentTransitionPrefixToken() const;

    /** @brief Returns the identifier for a line section at the current number/suffix. */
    QString lineId() const;

    /** @brief Returns the identifier for a transition section at the current number/suffix. */
    QString transitionId() const;

    /** @brief Returns the identifier of the current section (line or transition). */
    QString currentSectionId() const;

    /** @brief Returns the human-readable label of the current section kind ("Line" or "Transition"). */
    QString currentSectionLabel() const;

    /** @brief Advances to the next section (Transition → Line or Line → next Transition). */
    void goNext();

    /** @brief Retreats to the previous section (Line → Transition or Transition → previous Line). */
    void goPrevious();

    // ---- Public data members (part of the persistent state) ------------------

    QString linePrefix = QStringLiteral("L");          ///< Prefix for line sections (e.g. "L").
    QString transitionPrefix = QStringLiteral("TL");   ///< Prefix for transition sections (e.g. "TL").
    int number = 1;                                    ///< Current line/transition number (>= 1).
    QString suffix;                                    ///< Optional version suffix (e.g. "A", "B").
    SectionKind currentKind = SectionKind::Transition; ///< Kind of the current section.
};

// =====================================================================================================================

} // namespace sbpshots::core
