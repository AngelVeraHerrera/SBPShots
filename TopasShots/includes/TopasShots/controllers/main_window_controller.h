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
 * @file    main_window_controller.h
 * @brief   Controller layer between the main window UI and the core capture/storage services.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// TOPASSHOTS INCLUDES
#include "TopasShots/capture/capture_target.h"
#include "TopasShots/core/app_settings.h"
#include "TopasShots/core/shot_storage.h"

// QT INCLUDES
#include <QList>
#include <QPixmap>
#include <QRect>
#include <QString>

// =====================================================================================================================

namespace tshots::controllers
{

// =====================================================================================================================

/**
 * @brief Holds a pair of captured images (full frame and ROI crop).
 */
struct CapturePair
{
    QPixmap full;           ///< Full-frame capture.
    QPixmap crop;           ///< ROI-cropped capture.
    QString errorMessage;   ///< Non-empty if the capture failed.
};

// =====================================================================================================================

/**
 * @brief Mediates between the main window UI and the core capture and storage services.
 *
 * Owns the application settings, manages the campaign sequence, and orchestrates
 * capture and file-save operations on behalf of the UI.
 */
class MainWindowController final
{
public:

    /** @brief Constructor. Loads settings from disk on construction. */
    MainWindowController();

    /** @brief Returns a const reference to the current application settings. */
    const tshots::core::ApplicationSettings &settings() const;

    /** @brief Returns a mutable reference to the current application settings. */
    tshots::core::ApplicationSettings &settings();

    /** @brief Loads settings from the INI configuration file. */
    void load();

    /** @brief Saves the current settings to the INI configuration file. */
    void save() const;

    /** @brief Normalises the campaign, sequence prefixes, and line number. */
    void normalizeSequence();

    /** @brief Returns a human-readable label for the current section (id + kind). */
    QString currentSectionText() const;

    /** @brief Advances the sequence to the next section. */
    void nextSection();

    /** @brief Retreats the sequence to the previous section. */
    void previousSection();

    /**
     * @brief Returns true when all fields required for saving a campaign shot are valid.
     * @param missing Optional; receives the list of missing/invalid field names.
     */
    bool hasValidBaseConfiguration(QStringList *missing = nullptr) const;

    /** @brief Returns true when the ROI is configured and has a non-trivial size. */
    bool hasValidRoi() const;

    /** @brief Clears the current ROI. */
    void clearRoi();

    /**
     * @brief Enumerates all available capture targets, excluding the caller's own window.
     * @param selfWindowId Native handle of the caller's window.
     */
    QList<tshots::capture::CaptureTarget> enumerateTargets(quintptr selfWindowId) const;

    /**
     * @brief Sets a new capture target and clears the current ROI.
     * @param target New capture target.
     */
    void setCaptureTarget(const tshots::capture::CaptureTarget &target);

    /**
     * @brief Refreshes the window target if the stored handle is stale.
     * @return True if the target is valid (or was successfully refreshed).
     */
    bool refreshWindowTargetIfNeeded();

    /** @brief Returns true if the current window target is still visible. */
    bool isWindowTargetAvailable() const;

    /** @brief Falls back to the primary screen as the capture target. */
    void fallbackToPrimaryScreen();

    /**
     * @brief Captures the current target and returns both the full frame and ROI crop.
     * @return CapturePair with results or an error message.
     */
    CapturePair captureCurrentTarget() const;

    /**
     * @brief Saves a full/crop shot pair under the current campaign directory structure.
     * @param source       Shot origin (manual, auto, independent).
     * @param marker       Optional filename marker string.
     * @param full         Full-frame pixmap.
     * @param crop         ROI-crop pixmap.
     * @param paths        Optional; receives the saved file paths.
     * @param errorMessage Optional; receives a description on failure.
     * @return True on success.
     */
    bool saveCampaignShotPair(tshots::core::ShotSource source,
                              const QString &marker,
                              const QPixmap &full,
                              const QPixmap &crop,
                              tshots::core::SavedShotPaths *paths,
                              QString *errorMessage) const;

    /**
     * @brief Saves a full/crop shot pair to an arbitrary output folder.
     * @param folder       Destination directory path.
     * @param marker       Optional filename marker string.
     * @param full         Full-frame pixmap.
     * @param crop         ROI-crop pixmap.
     * @param paths        Optional; receives the saved file paths.
     * @param errorMessage Optional; receives a description on failure.
     * @return True on success.
     */
    bool saveIndependentShotPair(const QString &folder,
                                 const QString &marker,
                                 const QPixmap &full,
                                 const QPixmap &crop,
                                 tshots::core::SavedShotPaths *paths,
                                 QString *errorMessage) const;

private:

    tshots::core::ApplicationSettings settings_;
};

// =====================================================================================================================

} // namespace lst::controllers
