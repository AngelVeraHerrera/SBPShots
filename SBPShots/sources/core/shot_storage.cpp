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
 * @file    shot_storage.cpp
 * @brief   Screenshot file storage and campaign directory management.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/core/shot_storage.h"
#include "SBPShots/core/path_tokens.h"

// QT INCLUDES
#include <QDateTime>
#include <QDir>

// =====================================================================================================================

namespace sbpshots::core {

QString ShotStorage::shotDirectory(const QString &basePath,
                                   const QString &campaign,
                                   const QString &sectionId,
                                   ShotSource source,
                                   FrameKind frame)
{
    QString path = QDir(basePath).filePath(PathTokens::safePathToken(campaign));
    path = QDir(path).filePath(PathTokens::safePathToken(sectionId));
    path = QDir(path).filePath(ShotFileNamer::sourceToken(source));
    path = QDir(path).filePath(ShotFileNamer::frameToken(frame));
    return path;
}

bool ShotStorage::saveShotPair(const QString &basePath,
                               const QString &campaign,
                               const QString &sectionId,
                               ShotSource source,
                               const QString &marker,
                               const QPixmap &full,
                               const QPixmap &crop,
                               SavedShotPaths *paths,
                               QString *errorMessage)
{
    if (full.isNull() || crop.isNull())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The screenshot or ROI crop is empty.");
        return false;
    }

    const QString fullDir = shotDirectory(basePath, campaign, sectionId, source, FrameKind::Full);
    const QString cropDir = shotDirectory(basePath, campaign, sectionId, source, FrameKind::Crop);

    if (!QDir().mkpath(fullDir) || !QDir().mkpath(cropDir))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The output folders could not be created.");
        return false;
    }

    ShotNamingContext fullCtx{campaign, sectionId, source, FrameKind::Full, marker};
    ShotNamingContext cropCtx{campaign, sectionId, source, FrameKind::Crop, marker};

    const QString fullPath = QDir(fullDir).filePath(ShotFileNamer::fileName(fullCtx));
    const QString cropPath = QDir(cropDir).filePath(ShotFileNamer::fileName(cropCtx));

    if (!full.save(fullPath, "JPG") || !crop.save(cropPath, "JPG"))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The screenshot files could not be saved.");
        return false;
    }

    if (paths)
    {
        paths->fullPath = fullPath;
        paths->cropPath = cropPath;
    }

    return true;
}

bool ShotStorage::saveIndependentShotPair(const QString &folder,
                                          const QString &marker,
                                          const QPixmap &full,
                                          const QPixmap &crop,
                                          SavedShotPaths *paths,
                                          QString *errorMessage)
{
    if (folder.trimmed().isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The output folder is empty.");
        return false;
    }

    if (full.isNull() || crop.isNull())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The independent screenshot or ROI crop is empty.");
        return false;
    }

    const QString fullDir = QDir(folder).filePath(QStringLiteral("full"));
    const QString cropDir = QDir(folder).filePath(QStringLiteral("crop"));

    if (!QDir().mkpath(fullDir) || !QDir().mkpath(cropDir))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The independent output folders could not be created.");
        return false;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss_zzz");
    const QString markerToken = PathTokens::markerToken(marker);
    const QString suffix = markerToken.isEmpty() ? QString() : QStringLiteral("_%1").arg(markerToken);

    const QString fullPath = QDir(fullDir).filePath(QStringLiteral("independent_full_%1%2.JPG").arg(timestamp, suffix));
    const QString cropPath = QDir(cropDir).filePath(QStringLiteral("independent_crop_%1%2.JPG").arg(timestamp, suffix));

    if (!full.save(fullPath, "JPG") || !crop.save(cropPath, "JPG"))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("The independent screenshot files could not be saved.");
        return false;
    }

    if (paths)
    {
        paths->fullPath = fullPath;
        paths->cropPath = cropPath;
    }

    return true;
}

} // namespace sbpshots::core
