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
 * @file    main_window_controller.cpp
 * @brief   Controller layer between the main window UI and the core capture/storage services.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/controllers/main_window_controller.h"
#include "SBPShots/capture/capture_target_enumerator.h"
#include "SBPShots/capture/screenshot_service.h"
#include "SBPShots/core/path_tokens.h"

// =====================================================================================================================

namespace sbpshots::controllers {

MainWindowController::MainWindowController()
{
    load();
}

const sbpshots::core::ApplicationSettings &MainWindowController::settings() const
{
    return settings_;
}

sbpshots::core::ApplicationSettings &MainWindowController::settings()
{
    return settings_;
}

void MainWindowController::load()
{
    settings_ = sbpshots::core::AppSettingsStore::load();
}

void MainWindowController::save() const
{
    sbpshots::core::AppSettingsStore::save(settings_);
}

void MainWindowController::normalizeSequence()
{
    settings_.campaign = sbpshots::core::PathTokens::requiredPathToken(settings_.campaign);
    settings_.sequence.normalize();
}

QString MainWindowController::currentSectionText() const
{
    return QStringLiteral("%1 - %2")
        .arg(settings_.sequence.currentSectionId(), settings_.sequence.currentSectionLabel());
}

void MainWindowController::nextSection()
{
    settings_.sequence.goNext();
}

void MainWindowController::previousSection()
{
    settings_.sequence.goPrevious();
}

bool MainWindowController::hasValidBaseConfiguration(QStringList *missing) const
{
    QStringList localMissing;

    if (settings_.basePath.trimmed().isEmpty())
        localMissing << QStringLiteral("base path");

    if (sbpshots::core::PathTokens::requiredPathToken(settings_.campaign).isEmpty())
        localMissing << QStringLiteral("campaign");

    if (settings_.sequence.currentLinePrefixToken().isEmpty()
        || settings_.sequence.currentTransitionPrefixToken().isEmpty()
        || settings_.sequence.number <= 0
        || settings_.sequence.currentSectionId().isEmpty())
    {
        localMissing << QStringLiteral("line/transition sequence");
    }

    if (!hasValidRoi())
        localMissing << QStringLiteral("ROI");

    if (missing)
        *missing = localMissing;

    return localMissing.isEmpty();
}

bool MainWindowController::hasValidRoi() const
{
    return settings_.roi.isValid()
        && !settings_.roi.isEmpty()
        && settings_.roi.width() > 5
        && settings_.roi.height() > 5;
}

void MainWindowController::clearRoi()
{
    settings_.roi = QRect();
}

QList<sbpshots::capture::CaptureTarget> MainWindowController::enumerateTargets(quintptr selfWindowId) const
{
    return sbpshots::capture::CaptureTargetEnumerator::enumerateTargets(selfWindowId);
}

void MainWindowController::setCaptureTarget(const sbpshots::capture::CaptureTarget &target)
{
    settings_.captureTarget = target;
    clearRoi();
}

bool MainWindowController::refreshWindowTargetIfNeeded()
{
    if (settings_.captureTarget.type != sbpshots::capture::CaptureTargetType::Window)
        return true;

    if (sbpshots::capture::CaptureTargetEnumerator::isWindowAvailable(settings_.captureTarget.windowId))
        return true;

    sbpshots::capture::CaptureTarget target;
    if (sbpshots::capture::CaptureTargetEnumerator::findWindowByTitle(settings_.captureTarget.persistentName, &target))
    {
        settings_.captureTarget = target;
        return true;
    }

    return false;
}

bool MainWindowController::isWindowTargetAvailable() const
{
    if (settings_.captureTarget.type != sbpshots::capture::CaptureTargetType::Window)
        return true;

    return sbpshots::capture::CaptureTargetEnumerator::isWindowAvailable(settings_.captureTarget.windowId);
}

void MainWindowController::fallbackToPrimaryScreen()
{
    settings_.captureTarget = sbpshots::capture::CaptureTargetEnumerator::primaryScreenTarget();
    clearRoi();
}

CapturePair MainWindowController::captureCurrentTarget() const
{
    const auto result = sbpshots::capture::ScreenshotService::capture(settings_.captureTarget,
                                                                  settings_.roi);
    return CapturePair{result.full, result.crop, result.errorMessage};
}

bool MainWindowController::saveCampaignShotPair(sbpshots::core::ShotSource source,
                                                const QString &marker,
                                                const QPixmap &full,
                                                const QPixmap &crop,
                                                sbpshots::core::SavedShotPaths *paths,
                                                QString *errorMessage) const
{
    return sbpshots::core::ShotStorage::saveShotPair(settings_.basePath,
                                                settings_.campaign,
                                                settings_.sequence.currentSectionId(),
                                                source,
                                                marker,
                                                full,
                                                crop,
                                                paths,
                                                errorMessage);
}

bool MainWindowController::saveIndependentShotPair(const QString &folder,
                                                   const QString &marker,
                                                   const QPixmap &full,
                                                   const QPixmap &crop,
                                                   sbpshots::core::SavedShotPaths *paths,
                                                   QString *errorMessage) const
{
    return sbpshots::core::ShotStorage::saveIndependentShotPair(folder,
                                                           marker,
                                                           full,
                                                           crop,
                                                           paths,
                                                           errorMessage);
}

} // namespace sbpshots::controllers
