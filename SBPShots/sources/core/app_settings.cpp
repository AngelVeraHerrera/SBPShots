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
 * @file    app_settings.cpp
 * @brief   Application settings data structure and INI-based persistence.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/core/app_settings.h"
#include "SBPShots/capture/capture_target_enumerator.h"
#include "SBPShots/core/path_tokens.h"

// QT INCLUDES
#include <QCoreApplication>
#include <QSettings>

// =====================================================================================================================

namespace sbpshots::core {

ApplicationSettings AppSettingsStore::load()
{
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/config.ini");
    QSettings settings(path, QSettings::IniFormat);

    ApplicationSettings app;
    app.basePath = settings.value("Paths/path").toString();
    app.campaign = settings.value("Campaign/name").toString();

    app.sequence.linePrefix = settings.value("Line/prefix", "L").toString();
    app.sequence.transitionPrefix = settings.value("Transition/prefix", "TL").toString();
    app.sequence.number = settings.value("Line/number", 1).toInt();
    app.sequence.suffix = settings.value("Line/suffix").toString();
    app.sequence.currentKind = settings.value("Section/is_transition", true).toBool()
        ? SectionKind::Transition
        : SectionKind::Line;
    app.sequence.normalize();

    app.roi = QRect(settings.value("ROI/x", 0).toInt(),
                    settings.value("ROI/y", 0).toInt(),
                    settings.value("ROI/w", 0).toInt(),
                    settings.value("ROI/h", 0).toInt());

    app.alwaysOnTop = settings.value("Window/always_on_top", false).toBool();

    const QString targetType = settings.value("CaptureTarget/type", "screen").toString();
    app.captureTarget.type = targetType == "window"
        ? sbpshots::capture::CaptureTargetType::Window
        : sbpshots::capture::CaptureTargetType::Screen;

    app.captureTarget.screenIndex = settings.value("CaptureTarget/screen_index", 0).toInt();
    app.captureTarget.displayName = settings.value("CaptureTarget/name").toString();
    app.captureTarget.persistentName = settings.value("CaptureTarget/window_title").toString();
    app.captureTarget.windowId = 0;

    if (app.captureTarget.type == sbpshots::capture::CaptureTargetType::Window)
    {
        sbpshots::capture::CaptureTarget resolved;
        if (sbpshots::capture::CaptureTargetEnumerator::findWindowByTitle(app.captureTarget.persistentName, &resolved))
        {
            app.captureTarget = resolved;
        }
        else
        {
            app.captureTarget = sbpshots::capture::CaptureTargetEnumerator::primaryScreenTarget();
        }
    }
    else
    {
        const QList<sbpshots::capture::CaptureTarget> targets =
            sbpshots::capture::CaptureTargetEnumerator::enumerateTargets();

        bool found = false;
        for (const auto &target : targets)
        {
            if (target.type == sbpshots::capture::CaptureTargetType::Screen
                && target.screenIndex == app.captureTarget.screenIndex)
            {
                app.captureTarget = target;
                found = true;
                break;
            }
        }

        if (!found)
            app.captureTarget = sbpshots::capture::CaptureTargetEnumerator::primaryScreenTarget();
    }

    if (app.captureTarget.displayName.isEmpty())
        app.captureTarget = sbpshots::capture::CaptureTargetEnumerator::primaryScreenTarget();

    return app;
}

void AppSettingsStore::save(const ApplicationSettings &settingsData)
{
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/config.ini");
    QSettings settings(path, QSettings::IniFormat);

    CampaignSequence sequence = settingsData.sequence;
    sequence.normalize();

    settings.setValue("Paths/path", settingsData.basePath);
    settings.setValue("Campaign/name", PathTokens::requiredPathToken(settingsData.campaign));

    settings.setValue("Line/prefix", sequence.currentLinePrefixToken());
    settings.setValue("Transition/prefix", sequence.currentTransitionPrefixToken());
    settings.setValue("Line/number", sequence.number);
    settings.setValue("Line/suffix", sequence.currentSuffixToken());
    settings.setValue("Section/is_transition", sequence.currentKind == SectionKind::Transition);

    settings.setValue("ROI/enabled", settingsData.roi.isValid() && !settingsData.roi.isEmpty());
    settings.setValue("ROI/x", settingsData.roi.x());
    settings.setValue("ROI/y", settingsData.roi.y());
    settings.setValue("ROI/w", settingsData.roi.width());
    settings.setValue("ROI/h", settingsData.roi.height());

    settings.setValue("Window/always_on_top", settingsData.alwaysOnTop);

    settings.setValue("CaptureTarget/type",
                      settingsData.captureTarget.type == sbpshots::capture::CaptureTargetType::Window
                          ? "window"
                          : "screen");
    settings.setValue("CaptureTarget/name", settingsData.captureTarget.displayName);
    settings.setValue("CaptureTarget/screen_index", settingsData.captureTarget.screenIndex);
    settings.setValue("CaptureTarget/window_title",
                      settingsData.captureTarget.type == sbpshots::capture::CaptureTargetType::Window
                          ? settingsData.captureTarget.persistentName
                          : QString());
}

} // namespace sbpshots::core
