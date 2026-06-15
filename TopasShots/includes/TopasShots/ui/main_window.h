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
 * @file    main_window.h
 * @brief   Main application window for the TopasShots capture tool.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QDateTime>
#include <QDir>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

// TOPASSHOTS INCLUDES
#include "TopasShots/controllers/main_window_controller.h"

// =====================================================================================================================

class ImageViewer;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class RoiSelector;

namespace Ui {
class MainWindow;
}

// =====================================================================================================================

/**
 * @brief Main application window.
 *
 * Hosts all user controls for campaign configuration, manual and automatic screenshot capture,
 * ROI selection, section navigation, free-snip capture, and image preview.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:

    void resizeEvent(QResizeEvent *event) override;

private:

    /** @brief Identifies which preview image is currently displayed in the preview label. */
    enum class PreviewImageKind
    {
        ManualFull,   ///< Last manual full-frame capture.
        ManualCrop,   ///< Last manual ROI-crop capture.
        AutoFull,     ///< Last automatic full-frame capture.
        AutoCrop      ///< Last automatic ROI-crop capture.
    };

    // -- Setup ------------------------------------------------------------

    void configureUi();
    void configureConnections();

    // -- Settings sync ----------------------------------------------------

    void syncUiFromController();
    void syncControllerFromUi();
    void saveSettings();

    // -- UI helpers -------------------------------------------------------

    void applyAlwaysOnTop(bool enabled);
    void setCriticalConfigurationEnabled(bool enabled);
    void setLineEditError(QLineEdit *lineEdit, bool error);
    void updateConfigurationErrors();

    QString requestOptionalComment(const QString &title, const QString &label);

    // -- Capture target ---------------------------------------------------

    void configureCaptureTarget();
    void updateCaptureTargetUi();

    // -- ROI --------------------------------------------------------------

    void configureRoi();
    void updateRoiUi();

    // -- Section navigation -----------------------------------------------

    void updateCurrentSectionUi();
    void goToNextSection();
    void goToPreviousSection();
    void changeSection(bool forward);
    void goToNextVersion();
    QString nextVersionSuffix(const QString &suffix) const;

    // -- Automatic capture ------------------------------------------------

    void startAuto();
    void startPeriodicAuto();
    void stopAutomaticCapture(bool updateStartTime);
    bool isAutomaticCaptureActive() const;
    void updateAutoActionButtons();
    void updateAutoCaptureCheckText();

    // -- Timers and countdown ---------------------------------------------

    void updateCurrentUtcDateTime();
    void resetNextShotCountdown();
    void updateNextShotCountdown();

    // -- Target watchdog --------------------------------------------------

    void startTargetWatchdogIfNeeded();
    void stopTargetWatchdog();
    void checkTargetWatchdog();

    // -- Capture actions --------------------------------------------------

    bool takeShot(bool manual,
                  const QString &marker = QString(),
                  bool askManualComment = true);

    bool saveAutoMarkerShot(const QString &marker);
    void saveAuto();
    void saveManual();
    void saveIndependentShot();
    void forceNextAutoShot();

    // -- Free-snip --------------------------------------------------------

    void takeFreeSnip();
    void closeFreeSnipSelectors();
    void startFreeSnipSelector(bool restoreWindowAfterSelection);
    void finishFreeSnip(const QRect &globalRect, bool restoreWindowAfterSelection);

    QRect virtualDesktopGeometry() const;
    QPixmap grabGlobalScreenRoi(const QRect &globalRect) const;

    QString requestFreeSnipComment();
    QString buildFreeSnipFileName(const QString &comment) const;
    QString makeFreeSnipToken(const QString &text) const;

    // -- Preview image ----------------------------------------------------

    void updateManualScreenshotLabel();
    void updateBackupScreenshotLabel();

    void showLastManualCrop();
    void showLastAutoCrop();
    void showLastManualFull();
    void showLastAutoFull();

    void openLastManualFullInViewer();
    void openLastManualCropInViewer();
    void openLastAutoFullInViewer();
    void openLastAutoCropInViewer();

    void openPixmapInViewer(const QPixmap &pixmap,
                            const QString &title,
                            const QString &emptyMessage);

    void setPreviewImage(PreviewImageKind kind);
    void updatePreviewImageLabel();
    QPixmap currentPreviewPixmap() const;

    // -- Auto-shot helpers ------------------------------------------------

    QString currentSectionIdToken() const;
    QDir currentAutoShotDir(const QString &frameType) const;
    bool directoryContainsImages(const QDir &dir) const;
    bool currentAutoFolderIsEmpty() const;
    bool currentAutoMarkerExists(const QString &marker) const;
    QString automaticMarkerForNextShot() const;
    bool ensureEndMarkerShotForCurrentSection(bool askIfMissing);

    // -- Capture warning --------------------------------------------------

    void updateWarningControlsState();
    bool shouldCaptureWarningTimerRun() const;
    void resetCaptureWarningCountdown();
    void closeCaptureWarningPopup();
    void startCaptureWarningTimerIfNeeded();
    void stopCaptureWarningTimer();
    void updateCaptureWarningTimer();
    void checkCaptureWarning();
    void showCaptureWarningPopup(qint64 elapsedSeconds);

    // -- Misc helpers -----------------------------------------------------

    QPushButton *independentShotButton() const;

private:

    Ui::MainWindow *ui_ = nullptr;
    tshots::controllers::MainWindowController controller_;

    ImageViewer *viewer_ = nullptr;

    QTimer t_auto_;
    QTimer t_next_shot_;
    QTimer t_current_time_;
    QTimer t_capture_target_watchdog_;
    QTimer t_capture_warning_;

    QPixmap manual_shot_full_;
    QPixmap manual_shot_crop_;
    QPixmap auto_shot_full_;
    QPixmap auto_shot_crop_;

    QList<QPointer<RoiSelector>> free_snip_selectors_;
    bool free_snip_selection_active_ = false;

    QDateTime capture_warning_countdown_start_utc_;
    QPointer<QMessageBox> capture_warning_popup_;

    PreviewImageKind preview_image_kind_ = PreviewImageKind::ManualFull;
};
