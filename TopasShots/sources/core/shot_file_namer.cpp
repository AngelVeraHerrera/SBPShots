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
 * @file    shot_file_namer.cpp
 * @brief   Generates context-aware, timestamped filenames for captured screenshot pairs.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// TOPASSHOTS INCLUDES
#include "TopasShots/core/shot_file_namer.h"
#include "TopasShots/core/path_tokens.h"

// QT INCLUDES
#include <QDateTime>

// =====================================================================================================================

namespace tshots::core {

QString ShotFileNamer::sourceToken(ShotSource source)
{
    switch (source)
    {
    case ShotSource::Manual:
        return QStringLiteral("manual");
    case ShotSource::Auto:
        return QStringLiteral("auto");
    case ShotSource::Independent:
        return QStringLiteral("independent");
    }

    return QStringLiteral("shot");
}

QString ShotFileNamer::frameToken(FrameKind frame)
{
    switch (frame)
    {
    case FrameKind::Full:
        return QStringLiteral("full");
    case FrameKind::Crop:
        return QStringLiteral("crop");
    }

    return QStringLiteral("frame");
}

QString ShotFileNamer::fileName(const ShotNamingContext &context)
{
    const QString campaign = PathTokens::safePathToken(context.campaign);
    const QString section = PathTokens::safePathToken(context.sectionId);
    const QString source = sourceToken(context.source);
    const QString frame = frameToken(context.frame);
    const QString timestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss_zzz");
    const QString marker = PathTokens::markerToken(context.marker);

    if (marker.isEmpty())
        return QStringLiteral("%1_%2_%3_%4_%5.JPG").arg(campaign, section, source, frame, timestamp);

    return QStringLiteral("%1_%2_%3_%4_%5_%6.JPG").arg(campaign, section, source, frame, timestamp, marker);
}

} // namespace lst::core
