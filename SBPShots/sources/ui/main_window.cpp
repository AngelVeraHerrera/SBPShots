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
 * @file    main_window.cpp
 * @brief   Main application window for the SBP Shots capture tool.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// QT INCLUDES
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QColor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QScreen>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTime>
#include <QTimeZone>
#include <QTimer>
#include <QUrl>
#include <QStyle>

#include <utility>

// SBPSHOTS INCLUDES
#include "SBPShots/ui/main_window.h"
#include "SBPShots/capture/capture_target_enumerator.h"
#include "SBPShots/capture/screenshot_service.h"
#include "SBPShots/core/path_tokens.h"
#include "SBPShots/core/shot_types.h"
#include "SBPShots/widgets/roi_selector.h"
#include "SBPShots/viewers/image_viewer.h"
#include "ui_main_window.h"

namespace {

QPixmap makeDisabledPixmap(const QPixmap &normalPixmap)
{
    if (normalPixmap.isNull())
        return QPixmap();

    QImage image = normalPixmap.toImage().convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < image.height(); ++y)
    {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x)
        {
            const QColor src = QColor::fromRgba(line[x]);
            if (src.alpha() == 0)
                continue;

            const int gray = qGray(src.rgb());
            line[x] = QColor(gray, gray, gray, qMin(src.alpha(), 120)).rgba();
        }
    }

    return QPixmap::fromImage(image);
}

QIcon makeDisabledAwareIcon(const QIcon &sourceIcon, const QSize &iconSize)
{
    if (sourceIcon.isNull())
        return sourceIcon;

    const QSize effectiveSize = iconSize.isValid() && !iconSize.isEmpty()
                                    ? iconSize
                                    : QSize(24, 24);

    const QPixmap normalPixmap = sourceIcon.pixmap(effectiveSize, QIcon::Normal, QIcon::Off);
    if (normalPixmap.isNull())
        return sourceIcon;

    QIcon icon;
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    icon.addPixmap(makeDisabledPixmap(normalPixmap), QIcon::Disabled, QIcon::Off);
    return icon;
}

void fixButtonDisabledIcon(QPushButton *button)
{
    if (!button || button->icon().isNull())
        return;

    button->setIcon(makeDisabledAwareIcon(button->icon(), button->iconSize()));
}

} // namespace

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui_(new Ui::MainWindow),
    viewer_(new ImageViewer(this))
{
    ui_->setupUi(this);

    configureUi();
    syncUiFromController();

    t_auto_.setSingleShot(false);
    t_auto_.setInterval(qMax(1, ui_->sb_int_->value()) * 60000);

    t_next_shot_.setInterval(500);
    t_next_shot_.setTimerType(Qt::PreciseTimer);

    t_current_time_.setInterval(500);
    t_current_time_.setTimerType(Qt::PreciseTimer);

    t_capture_target_watchdog_.setInterval(2000);
    t_capture_target_watchdog_.setTimerType(Qt::CoarseTimer);

    t_capture_warning_.setInterval(5000);
    t_capture_warning_.setTimerType(Qt::CoarseTimer);

    capture_warning_countdown_start_utc_ = QDateTime::currentDateTimeUtc();

    updateCurrentUtcDateTime();
    resetNextShotCountdown();

    if (ui_->le_suffix_->text().trimmed().isEmpty())
        ui_->le_suffix_->setText(QStringLiteral("A"));

    updateAutoActionButtons();
    updateAutoCaptureCheckText();
    updateCaptureWarningTimer();

    const int msToNextSecond = 1000 - QDateTime::currentDateTimeUtc().time().msec();
    QTimer::singleShot(msToNextSecond, this, [this]() {
        updateCurrentUtcDateTime();
        t_current_time_.start();
    });

    configureConnections();
    updateConfigurationErrors();
    updatePreviewImageLabel();
}

MainWindow::~MainWindow()
{
    delete ui_;
}

void MainWindow::configureUi()
{
    setWindowTitle(tr("SBP Shots"));

    ui_->l_screen_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui_->l_screen_->setAlignment(Qt::AlignCenter);

    if (const QScreen *screen = QGuiApplication::primaryScreen())
        ui_->l_screen_->setMinimumSize(screen->geometry().width() / 8, screen->geometry().height() / 8);

    manual_shot_full_ = ui_->l_screen_->pixmap();
    auto_shot_full_ = ui_->l_backup_->pixmap();

    ui_->sb_nline_->setPaddingWidth(3);

    static const QRegularExpression safeNameRegex(R"([^<>:"/\\|?*\x00-\x1F]*)");
    static const QRegularExpression suffixRegex(QStringLiteral("[A-Za-z]*"));

    ui_->le_campaign_->setValidator(new QRegularExpressionValidator(safeNameRegex, ui_->le_campaign_));
    ui_->le_line_prefix_->setValidator(new QRegularExpressionValidator(safeNameRegex, ui_->le_line_prefix_));
    ui_->le_trans_prefix_->setValidator(new QRegularExpressionValidator(safeNameRegex, ui_->le_trans_prefix_));
    ui_->le_suffix_->setValidator(new QRegularExpressionValidator(suffixRegex, ui_->le_suffix_));

    ui_->le_campaign_->setMaxLength(64);
    ui_->le_line_prefix_->setMaxLength(8);
    ui_->le_trans_prefix_->setMaxLength(8);
    ui_->le_suffix_->setMaxLength(8);

    ui_->le_path_->setReadOnly(true);
    ui_->le_roi_->setReadOnly(true);
    ui_->le_current_section_->setReadOnly(true);
    ui_->le_capture_target_->setReadOnly(true);

    ui_->dt_current_time_->setDisplayFormat("yyyy-MM-dd HH:mm:ss 'UTC'");
    ui_->dt_current_time_->setTimeZone(QTimeZone::utc());
    ui_->dt_current_time_->setReadOnly(true);
    ui_->dt_current_time_->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui_->te_nshot_->setDisplayFormat("HH:mm:ss");
    ui_->te_nshot_->setReadOnly(true);
    ui_->te_nshot_->setButtonSymbols(QAbstractSpinBox::NoButtons);

    ui_->le_suffix_->setToolTip(tr("Mandatory line/transit version suffix, for example A, B, C."));
    ui_->pb_next_version_->setToolTip(tr("Increase the current line/transit version suffix, for example A to B."));
    ui_->pb_force_next_shot_->setToolTip(tr("Force an automatic capture now and restart the countdown."));
    ui_->pb_force_next_shot_->setEnabled(false);
    ui_->pb_free_snip_->setToolTip(tr("Select any screen area and save it manually to a chosen folder."));

    ui_->cb_back_->setText(ui_->cb_back_->isChecked() ? tr("AUTO ON") : tr("AUTO OFF"));
    ui_->cb_back_->setToolTip(tr("Enable or disable automatic campaign captures."));

    ui_->sb_int_->setToolTip(tr("Time between automatic captures. The first automatic capture is taken after this interval."));

    ui_->cb_warnings_->setText(tr("Alerts"));
    ui_->cb_warnings_->setToolTip(tr("Show a warning if no campaign captures have been saved for the configured watchdog time."));
    ui_->sb_warn_->setToolTip(tr("Watchdog time in minutes. A warning is shown if no campaign capture is saved during this time."));
    ui_->sb_warn_->setSuffix(tr(" min"));
    updateWarningControlsState();

    ui_->rb_open_lm_full_->setToolTip(tr("Show last manual full capture."));
    ui_->rb_open_lm_crop_->setToolTip(tr("Show last manual crop capture."));
    ui_->rb_open_la_full_->setToolTip(tr("Show last automatic full capture."));
    ui_->rb_open_la_crop_->setToolTip(tr("Show last automatic crop capture."));
    ui_->pb_open_lm_full_->setToolTip(tr("Open last manual full capture in the image viewer."));
    ui_->pb_open_lm_crop_->setToolTip(tr("Open last manual crop capture in the image viewer."));
    ui_->pb_open_la_full_->setToolTip(tr("Open last automatic full capture in the image viewer."));
    ui_->pb_open_la_crop_->setToolTip(tr("Open last automatic crop capture in the image viewer."));

    if (!ui_->rb_open_lm_full_->isChecked()
        && !ui_->rb_open_lm_crop_->isChecked()
        && !ui_->rb_open_la_full_->isChecked()
        && !ui_->rb_open_la_crop_->isChecked())
    {
        ui_->rb_open_lm_full_->setChecked(true);
    }

    preview_image_kind_ = PreviewImageKind::ManualFull;

    if (QPushButton *button = independentShotButton())
    {
        button->setText(tr("Ad-hoc Capture"));
        button->setToolTip(tr("Take an independent capture and choose its output folder after capture."));
    }

    for (QPushButton *button : this->findChildren<QPushButton *>())
        fixButtonDisabledIcon(button);
}

void MainWindow::syncUiFromController()
{
    const auto &settings = controller_.settings();

    ui_->le_path_->setText(settings.basePath);
    ui_->le_campaign_->setText(settings.campaign);
    ui_->le_line_prefix_->setText(settings.sequence.linePrefix);
    ui_->le_trans_prefix_->setText(settings.sequence.transitionPrefix);
    ui_->sb_nline_->setValue(settings.sequence.number);
    ui_->le_suffix_->setText(settings.sequence.suffix.trimmed().isEmpty()
                                 ? QStringLiteral("A")
                                 : settings.sequence.suffix.trimmed().toUpper());
    ui_->cb_alwaystop_->setChecked(settings.alwaysOnTop);

    applyAlwaysOnTop(settings.alwaysOnTop);
    updateCaptureTargetUi();
    updateCurrentSectionUi();
    updateRoiUi();
}

void MainWindow::syncControllerFromUi()
{
    auto &settings = controller_.settings();

    settings.basePath = ui_->le_path_->text();
    settings.campaign = ui_->le_campaign_->text();
    settings.sequence.linePrefix = ui_->le_line_prefix_->text();
    settings.sequence.transitionPrefix = ui_->le_trans_prefix_->text();
    settings.sequence.number = ui_->sb_nline_->value();
    settings.sequence.suffix = ui_->le_suffix_->text().trimmed().isEmpty()
                                   ? QStringLiteral("A")
                                   : ui_->le_suffix_->text().trimmed().toUpper();
    settings.alwaysOnTop = ui_->cb_alwaystop_->isChecked();

    controller_.normalizeSequence();

    ui_->le_campaign_->setText(settings.campaign);
    ui_->le_line_prefix_->setText(settings.sequence.linePrefix);
    ui_->le_trans_prefix_->setText(settings.sequence.transitionPrefix);
    ui_->sb_nline_->setValue(settings.sequence.number);
    ui_->le_suffix_->setText(settings.sequence.suffix);
}

void MainWindow::saveSettings()
{
    syncControllerFromUi();
    controller_.save();
}

void MainWindow::configureConnections()
{
    connect(ui_->pb_take_, &QPushButton::clicked, this, [this]() { takeShot(true); });
    connect(ui_->pb_config_roi_, &QPushButton::clicked, this, &MainWindow::configureRoi);
    connect(ui_->pb_config_target_, &QPushButton::clicked, this, &MainWindow::configureCaptureTarget);

    if (QPushButton *button = independentShotButton())
        connect(button, &QPushButton::clicked, this, &MainWindow::saveIndependentShot);

    connect(ui_->pb_prev_section_, &QPushButton::clicked, this, &MainWindow::goToPreviousSection);
    connect(ui_->pb_next_section_, &QPushButton::clicked, this, &MainWindow::goToNextSection);
    connect(ui_->pb_next_version_, &QPushButton::clicked, this, &MainWindow::goToNextVersion);
    connect(ui_->pb_force_next_shot_, &QPushButton::clicked, this, &MainWindow::forceNextAutoShot);

    connect(ui_->pb_open_folder_, &QPushButton::clicked, this, [this]() {
        const QString folderPath = QDir(ui_->le_path_->text()).filePath(ui_->le_campaign_->text());
        if (!QDir(folderPath).exists())
        {
            QMessageBox::warning(this, tr("Folder not found"), tr("The folder has not been created yet."));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    });

    connect(ui_->pb_getfolder_, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this,
                                                              tr("Select base screenshot folder"),
                                                              ui_->le_path_->text());
        if (!dir.isEmpty())
        {
            ui_->le_path_->setText(QDir::toNativeSeparators(dir));
            saveSettings();
            updateConfigurationErrors();
        }
    });

    auto normalizeAndSave = [this]() {
        saveSettings();
        updateCurrentSectionUi();
        updateConfigurationErrors();
    };

    connect(ui_->le_campaign_, &QLineEdit::editingFinished, this, normalizeAndSave);
    connect(ui_->le_line_prefix_, &QLineEdit::editingFinished, this, normalizeAndSave);
    connect(ui_->le_trans_prefix_, &QLineEdit::editingFinished, this, normalizeAndSave);
    connect(ui_->le_suffix_, &QLineEdit::editingFinished, this, normalizeAndSave);

    connect(ui_->le_suffix_, &QLineEdit::textChanged, this, [this](const QString &text) {
        controller_.settings().sequence.suffix = text.trimmed().isEmpty()
        ? QStringLiteral("A")
        : text.trimmed().toUpper();
        controller_.normalizeSequence();
        updateCurrentSectionUi();
        updateConfigurationErrors();
    });

    connect(ui_->sb_nline_, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int) {
        saveSettings();
        updateCurrentSectionUi();
        updateConfigurationErrors();
    });

    connect(ui_->cb_alwaystop_, &QCheckBox::toggled, this, [this](bool checked) {
        applyAlwaysOnTop(checked);
        saveSettings();
    });

    connect(ui_->cb_back_, &QCheckBox::toggled, this, [this](bool checked) {
        saveSettings();
        updateConfigurationErrors();
        updateAutoCaptureCheckText();

        if (checked)
        {
            QStringList missing;
            if (!controller_.hasValidBaseConfiguration(&missing))
            {
                QSignalBlocker blocker(ui_->cb_back_);
                ui_->cb_back_->setChecked(false);
                updateAutoCaptureCheckText();

                resetCaptureWarningCountdown();
                updateWarningControlsState();
                updateCaptureWarningTimer();

                QMessageBox::warning(this,
                                     tr("Automatic Capture Error"),
                                     tr("Missing or invalid configuration: %1.").arg(missing.join(", ")));
                updateAutoActionButtons();
                return;
            }
            startAuto();
        }
        else
        {
            const bool wasAutoActive = isAutomaticCaptureActive();
            if (wasAutoActive)
            {
                t_auto_.stop();
                t_next_shot_.stop();
                ensureEndMarkerShotForCurrentSection(true);
            }
            stopAutomaticCapture(false);
        }
        updateAutoActionButtons();
    });

    connect(ui_->sb_int_, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int value) {
        t_auto_.setInterval(qMax(1, value) * 60000);
        if (ui_->cb_back_->isChecked())
        {
            t_auto_.stop();
            t_auto_.start();
            t_next_shot_.start();
            updateNextShotCountdown();
        }
    });

    connect(ui_->cb_warnings_, &QCheckBox::toggled, this, [this](bool) {
        closeCaptureWarningPopup();
        resetCaptureWarningCountdown();
        updateWarningControlsState();
        updateCaptureWarningTimer();
    });

    connect(ui_->sb_warn_, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int) {
        closeCaptureWarningPopup();
        resetCaptureWarningCountdown();
        updateCaptureWarningTimer();
    });

    connect(ui_->pb_free_snip_, &QPushButton::clicked, this, &MainWindow::takeFreeSnip);

    connect(&t_auto_, &QTimer::timeout, this, [this]() {
        saveAuto();
        updateNextShotCountdown();
    });
    connect(&t_current_time_, &QTimer::timeout, this, &MainWindow::updateCurrentUtcDateTime);
    connect(&t_next_shot_, &QTimer::timeout, this, &MainWindow::updateNextShotCountdown);
    connect(&t_capture_target_watchdog_, &QTimer::timeout, this, &MainWindow::checkTargetWatchdog);
    connect(&t_capture_warning_, &QTimer::timeout, this, &MainWindow::checkCaptureWarning);

    connect(ui_->rb_open_lm_full_, &QRadioButton::clicked, this, [this](bool checked) {
        if (checked)
            setPreviewImage(PreviewImageKind::ManualFull);
    });
    connect(ui_->rb_open_lm_crop_, &QRadioButton::clicked, this, [this](bool checked) {
        if (checked)
            setPreviewImage(PreviewImageKind::ManualCrop);
    });
    connect(ui_->rb_open_la_full_, &QRadioButton::clicked, this, [this](bool checked) {
        if (checked)
            setPreviewImage(PreviewImageKind::AutoFull);
    });
    connect(ui_->rb_open_la_crop_, &QRadioButton::clicked, this, [this](bool checked) {
        if (checked)
            setPreviewImage(PreviewImageKind::AutoCrop);
    });

    connect(ui_->pb_open_lm_full_,
            &QPushButton::clicked,
            this,
            &MainWindow::openLastManualFullInViewer);

    connect(ui_->pb_open_lm_crop_,
            &QPushButton::clicked,
            this,
            &MainWindow::openLastManualCropInViewer);

    connect(ui_->pb_open_la_full_,
            &QPushButton::clicked,
            this,
            &MainWindow::openLastAutoFullInViewer);

    connect(ui_->pb_open_la_crop_,
            &QPushButton::clicked,
            this,
            &MainWindow::openLastAutoCropInViewer);
}

void MainWindow::openPixmapInViewer(const QPixmap &pixmap,
                                    const QString &title,
                                    const QString &emptyMessage)
{
    if (pixmap.isNull())
    {
        QMessageBox::warning(this,
                             title,
                             emptyMessage);
        return;
    }

    viewer_->setWindowTitle(title);
    viewer_->setPixmap(pixmap);
    viewer_->showMaximized();
    viewer_->raise();
    viewer_->activateWindow();
}

void MainWindow::openLastManualFullInViewer()
{
    openPixmapInViewer(manual_shot_full_,
                       tr("Last Manual Full Capture"),
                       tr("No manual full capture is available yet."));
}

void MainWindow::openLastManualCropInViewer()
{
    openPixmapInViewer(manual_shot_crop_,
                       tr("Last Manual Crop Capture"),
                       tr("No manual crop capture is available yet."));
}

void MainWindow::openLastAutoFullInViewer()
{
    openPixmapInViewer(auto_shot_full_,
                       tr("Last Auto Full Capture"),
                       tr("No automatic full capture is available yet."));
}

void MainWindow::openLastAutoCropInViewer()
{
    openPixmapInViewer(auto_shot_crop_,
                       tr("Last Auto Crop Capture"),
                       tr("No automatic crop capture is available yet."));
}

void MainWindow::takeFreeSnip()
{
    if (free_snip_selection_active_)
        return;

    const bool restoreWindowAfterSelection = this->isVisible();
    this->hide();

    QTimer::singleShot(250, this, [this, restoreWindowAfterSelection]() {
        this->startFreeSnipSelector(restoreWindowAfterSelection);
    });
}

void MainWindow::closeFreeSnipSelectors()
{
    for (const QPointer<RoiSelector> &selector : std::as_const(free_snip_selectors_))
    {
        if (!selector)
            continue;

        selector->hide();
        selector->close();
    }

    free_snip_selectors_.clear();
}

void MainWindow::startFreeSnipSelector(bool restoreWindowAfterSelection)
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    if (screens.isEmpty())
    {
        if (restoreWindowAfterSelection)
            this->show();

        QMessageBox::warning(this, tr("Free Snip"), tr("No screen is available."));
        return;
    }

    this->closeFreeSnipSelectors();
    free_snip_selection_active_ = true;

    for (QScreen *screen : screens)
    {
        if (!screen)
            continue;

        auto *selector = new RoiSelector(screen, QRect(), nullptr, false);
        free_snip_selectors_.append(QPointer<RoiSelector>(selector));

        connect(selector, &RoiSelector::roiSelected, this, [this, restoreWindowAfterSelection](const QRect &globalRect) {
            if (!free_snip_selection_active_)
                return;

            free_snip_selection_active_ = false;
            this->closeFreeSnipSelectors();

            const QRect selectedRect = globalRect.normalized();
            QTimer::singleShot(150, this, [this, selectedRect, restoreWindowAfterSelection]() {
                this->finishFreeSnip(selectedRect, restoreWindowAfterSelection);
            });
        });

        connect(selector, &RoiSelector::roiCanceled, this, [this, restoreWindowAfterSelection]() {
            if (!free_snip_selection_active_)
                return;

            free_snip_selection_active_ = false;
            this->closeFreeSnipSelectors();

            if (restoreWindowAfterSelection)
                this->show();
        });

        selector->show();
        selector->raise();
        selector->activateWindow();
    }
}

void MainWindow::finishFreeSnip(const QRect &globalRect,
                                bool restoreWindowAfterSelection)
{
    const QRect roi = globalRect.normalized();

    if (!roi.isValid() || roi.isEmpty() || roi.width() <= 5 || roi.height() <= 5)
    {
        if (restoreWindowAfterSelection)
            this->show();

        QMessageBox::warning(this, tr("Free Snip"), tr("Invalid capture area."));
        return;
    }

    const QPixmap snip = this->grabGlobalScreenRoi(roi);

    if (restoreWindowAfterSelection)
        this->show();

    if (snip.isNull())
    {
        QMessageBox::warning(this, tr("Free Snip"), tr("The selected area could not be captured."));
        return;
    }

    const QString outputDir = QFileDialog::getExistingDirectory(this,
                                                                tr("Select Free Snip Output Folder"),
                                                                QString());
    if (outputDir.isEmpty())
        return;

    const QString comment = this->requestFreeSnipComment();
    const QString fileName = this->buildFreeSnipFileName(comment);
    const QString filePath = QDir(outputDir).filePath(fileName);

    if (!snip.save(filePath))
    {
        QMessageBox::warning(this,
                             tr("Free Snip"),
                             tr("The image could not be saved to:\n%1").arg(QDir::toNativeSeparators(filePath)));
        return;
    }

    QMessageBox::information(this,
                             tr("Free Snip"),
                             tr("Image saved to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}

QRect MainWindow::virtualDesktopGeometry() const
{
    QRect geometry;

    for (const QScreen *screen : QGuiApplication::screens())
    {
        if (!screen)
            continue;

        geometry = geometry.isNull() ? screen->geometry() : geometry.united(screen->geometry());
    }

    return geometry;
}

QPixmap MainWindow::grabGlobalScreenRoi(const QRect &globalRect) const
{
    const QRect roi = globalRect.normalized();

    if (!roi.isValid() || roi.isEmpty())
        return QPixmap();

    QPixmap result(roi.size());
    result.fill(Qt::transparent);

    QPainter painter(&result);

    for (QScreen *screen : QGuiApplication::screens())
    {
        if (!screen)
            continue;

        const QRect screenGeometry = screen->geometry();
        const QRect intersection = roi.intersected(screenGeometry);

        if (!intersection.isValid() || intersection.isEmpty())
            continue;

        const QRect relativeToScreen = intersection.translated(-screenGeometry.topLeft());
        const QPixmap part = screen->grabWindow(0,
                                                relativeToScreen.x(),
                                                relativeToScreen.y(),
                                                relativeToScreen.width(),
                                                relativeToScreen.height());

        if (part.isNull())
            continue;

        const QRect targetRect = intersection.translated(-roi.topLeft());
        const qreal dpr = part.devicePixelRatio();
        const QRectF sourceRect(0.0, 0.0, part.width() / dpr, part.height() / dpr);

        painter.drawPixmap(targetRect, part, sourceRect);
    }

    painter.end();
    return result;
}

QString MainWindow::requestFreeSnipComment()
{
    bool accepted = false;
    const QString text = QInputDialog::getText(this,
                                               tr("Free Snip Comment"),
                                               tr("Optional comment for the filename:"),
                                               QLineEdit::Normal,
                                               QString(),
                                               &accepted);
    if (!accepted)
        return QString();

    return this->makeFreeSnipToken(text);
}

QString MainWindow::buildFreeSnipFileName(const QString &comment) const
{
    const QString timestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss_zzz");
    const QString marker = this->makeFreeSnipToken(comment);

    if (marker.isEmpty())
        return QStringLiteral("SBPShots_free-snip_%1.JPG").arg(timestamp);

    return QStringLiteral("SBPShots_free-snip_%1_%2.JPG").arg(timestamp, marker);
}

void MainWindow::applyAlwaysOnTop(bool enabled)
{
    Qt::WindowFlags flags = windowFlags();

    if (enabled)
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;

    const bool wasVisible = isVisible();
    setWindowFlags(flags);

    if (wasVisible)
        show();
}

void MainWindow::setCriticalConfigurationEnabled(bool enabled)
{
    ui_->pb_getfolder_->setEnabled(enabled);
    ui_->pb_config_target_->setEnabled(enabled);
    ui_->pb_config_roi_->setEnabled(enabled);

    ui_->le_campaign_->setEnabled(enabled);
    ui_->le_line_prefix_->setEnabled(enabled);
    ui_->le_trans_prefix_->setEnabled(enabled);
    ui_->le_suffix_->setEnabled(enabled);
    ui_->sb_nline_->setEnabled(enabled);

    ui_->pb_prev_section_->setEnabled(enabled);
    ui_->pb_next_section_->setEnabled(enabled);
    ui_->pb_next_version_->setEnabled(enabled);

    ui_->sb_int_->setEnabled(enabled);
    updateAutoActionButtons();
    updateWarningControlsState();
}

void MainWindow::setLineEditError(QLineEdit *lineEdit, bool error)
{
    if (!lineEdit)
        return;

    lineEdit->setProperty("error", error);
    lineEdit->style()->unpolish(lineEdit);
    lineEdit->style()->polish(lineEdit);
    lineEdit->update();
}

void MainWindow::updateConfigurationErrors()
{
    const auto &settings = controller_.settings();

    setLineEditError(ui_->le_path_, settings.basePath.trimmed().isEmpty());
    setLineEditError(ui_->le_campaign_, sbpshots::core::PathTokens::requiredPathToken(settings.campaign).isEmpty());
    setLineEditError(ui_->le_line_prefix_, settings.sequence.currentLinePrefixToken().isEmpty());
    setLineEditError(ui_->le_trans_prefix_, settings.sequence.currentTransitionPrefixToken().isEmpty());
    setLineEditError(ui_->le_suffix_, ui_->le_suffix_->text().trimmed().isEmpty());
    setLineEditError(ui_->le_roi_, !controller_.hasValidRoi());
}

QString MainWindow::requestOptionalComment(const QString &title, const QString &label)
{
    bool accepted = false;
    const QString text = QInputDialog::getText(this,
                                               title,
                                               label,
                                               QLineEdit::Normal,
                                               QString(),
                                               &accepted);
    if (!accepted)
        return QString();

    return sbpshots::core::PathTokens::markerToken(text);
}

void MainWindow::configureCaptureTarget()
{
    const WId selfWindowId = this->winId();
    const auto targets = controller_.enumerateTargets(static_cast<quintptr>(selfWindowId));

    if (targets.isEmpty())
    {
        QMessageBox::warning(this, tr("Capture Target"), tr("No capture targets are available."));
        return;
    }

    QStringList items;
    int currentIndex = 0;
    const auto &currentTarget = controller_.settings().captureTarget;

    for (int i = 0; i < targets.size(); ++i)
    {
        items << targets.at(i).displayName;
        if (targets.at(i).type == currentTarget.type)
        {
            if (targets.at(i).isScreen() && targets.at(i).screenIndex == currentTarget.screenIndex)
                currentIndex = i;
            else if (targets.at(i).isWindow() && targets.at(i).persistentName == currentTarget.persistentName)
                currentIndex = i;
        }
    }

    bool accepted = false;
    const QString selected = QInputDialog::getItem(this,
                                                   tr("Select Capture Target"),
                                                   tr("Capture target:"),
                                                   items,
                                                   currentIndex,
                                                   false,
                                                   &accepted);
    if (!accepted || selected.isEmpty())
        return;

    const int index = items.indexOf(selected);
    if (index < 0 || index >= targets.size())
        return;

    controller_.setCaptureTarget(targets.at(index));
    updateCaptureTargetUi();
    updateRoiUi();
    updateConfigurationErrors();
    controller_.save();
}

void MainWindow::updateCaptureTargetUi()
{
    const auto &target = controller_.settings().captureTarget;
    ui_->le_capture_target_->setText(target.displayName);
    ui_->le_capture_target_->setToolTip(target.displayName);
}

void MainWindow::updateCurrentSectionUi()
{
    ui_->le_current_section_->setText(controller_.currentSectionText());

    if (controller_.settings().sequence.currentKind == sbpshots::core::SectionKind::Transition)
    {
        ui_->le_current_section_->setStyleSheet(
            "QLineEdit {"
            "background-color: rgb(255, 225, 180);"
            "color: rgb(35, 35, 35);"
            "font: 14px \"Open Sans Semibold\";"
            "padding: 2px 2px;"
            "}");
    }
    else
    {
        ui_->le_current_section_->setStyleSheet(
            "QLineEdit {"
            "background-color: rgb(205, 245, 205);"
            "color: rgb(35, 35, 35);"
            "font: 14px \"Open Sans Semibold\";"
            "padding: 2px 2px;"
            "}");
    }

    ui_->le_current_section_->setToolTip(tr("Current section: %1").arg(controller_.currentSectionText()));
}

void MainWindow::configureRoi()
{
    const auto &target = controller_.settings().captureTarget;
    QScreen *screen = sbpshots::capture::ScreenshotService::screenForTarget(target);

    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (!screen)
    {
        QMessageBox::warning(this, tr("ROI Error"), tr("No screen is available to configure the ROI."));
        return;
    }

    QRect allowedSelectionGlobal;

    if (target.isWindow())
    {
        if (!controller_.isWindowTargetAvailable())
        {
            QMessageBox::warning(this, tr("ROI Error"), tr("The selected target window is not available."));
            return;
        }

        allowedSelectionGlobal = sbpshots::capture::CaptureTargetEnumerator::windowRect(target.windowId);
        if (!allowedSelectionGlobal.isValid() || allowedSelectionGlobal.isEmpty())
        {
            QMessageBox::warning(this, tr("ROI Error"), tr("The selected target window geometry is not valid."));
            return;
        }
    }

    auto *selector = new RoiSelector(screen, allowedSelectionGlobal);

    connect(selector, &RoiSelector::roiSelected, this, [this](const QRect &globalRect) {
        const auto &target = controller_.settings().captureTarget;

        if (target.isWindow())
        {
            const QRect targetRect = sbpshots::capture::CaptureTargetEnumerator::windowRect(target.windowId);
            const QRect roiOnTarget = globalRect.normalized().intersected(targetRect);

            if (!roiOnTarget.isValid() || roiOnTarget.isEmpty())
            {
                QMessageBox::warning(this, tr("ROI Error"), tr("The selected ROI is outside the target window."));
                return;
            }

            controller_.settings().roi = roiOnTarget.translated(-targetRect.topLeft());
        }
        else
        {
            controller_.settings().roi = globalRect.normalized();
        }

        updateRoiUi();
        updateConfigurationErrors();
        controller_.save();
    });

    connect(selector, &RoiSelector::roiCanceled, this, [this]() {
        updateRoiUi();
        updateConfigurationErrors();
    });

    selector->show();
    selector->raise();
    selector->activateWindow();
}

void MainWindow::updateRoiUi()
{
    const QRect roi = controller_.settings().roi;

    if (!controller_.hasValidRoi())
    {
        ui_->pb_config_roi_->setText(tr("ROI"));
        ui_->pb_config_roi_->setToolTip(tr("No ROI configured."));
        ui_->le_roi_->clear();
        ui_->le_roi_->setPlaceholderText(tr("No ROI configured"));
        return;
    }

    const QString roiText = QStringLiteral("x=%1, y=%2, w=%3, h=%4")
                                .arg(roi.x()).arg(roi.y()).arg(roi.width()).arg(roi.height());

    ui_->pb_config_roi_->setText(tr("ROI"));
    ui_->pb_config_roi_->setToolTip(roiText);
    ui_->le_roi_->setText(roiText);
    ui_->le_roi_->setToolTip(roiText);
}

void MainWindow::startAuto()
{
    t_auto_.stop();
    t_auto_.setInterval(qMax(1, ui_->sb_int_->value()) * 60000);
    t_auto_.start();

    t_next_shot_.start();

    startTargetWatchdogIfNeeded();
    updateNextShotCountdown();
    setCriticalConfigurationEnabled(false);
    updateAutoActionButtons();
    updateAutoCaptureCheckText();

    closeCaptureWarningPopup();
    updateWarningControlsState();
    updateCaptureWarningTimer();
}

void MainWindow::startPeriodicAuto()
{
    t_auto_.stop();
    t_auto_.setInterval(qMax(1, ui_->sb_int_->value()) * 60000);
    t_auto_.start();

    t_next_shot_.start();

    startTargetWatchdogIfNeeded();
    updateNextShotCountdown();
    setCriticalConfigurationEnabled(false);
    updateAutoActionButtons();
    updateAutoCaptureCheckText();

    closeCaptureWarningPopup();
    updateWarningControlsState();
    updateCaptureWarningTimer();
}

void MainWindow::stopAutomaticCapture(bool updateStartTime)
{
    Q_UNUSED(updateStartTime)

    t_auto_.stop();
    t_next_shot_.stop();

    stopTargetWatchdog();
    resetNextShotCountdown();
    setCriticalConfigurationEnabled(true);

    updateAutoActionButtons();
    updateAutoCaptureCheckText();

    resetCaptureWarningCountdown();
    updateWarningControlsState();
    updateCaptureWarningTimer();
}

bool MainWindow::isAutomaticCaptureActive() const
{
    return t_auto_.isActive() || t_next_shot_.isActive();
}

void MainWindow::updateAutoActionButtons()
{
    ui_->pb_force_next_shot_->setEnabled(isAutomaticCaptureActive());
}

void MainWindow::updateAutoCaptureCheckText()
{
    if (ui_->cb_back_->isChecked())
    {
        ui_->cb_back_->setText(tr("AUTO ACTIVE"));
        ui_->cb_back_->setToolTip(tr("Automatic campaign capture is active."));
    }
    else
    {
        ui_->cb_back_->setText(tr("AUTO OFF"));
        ui_->cb_back_->setToolTip(tr("Automatic campaign capture is disabled."));
    }
}

void MainWindow::updateCurrentUtcDateTime()
{
    ui_->dt_current_time_->setDateTime(QDateTime::currentDateTimeUtc());
}

void MainWindow::resetNextShotCountdown()
{
    ui_->te_nshot_->setTime(QTime(0, 0, 0));
}

void MainWindow::updateNextShotCountdown()
{
    if (!t_auto_.isActive())
    {
        resetNextShotCountdown();
        return;
    }

    int remainingMs = t_auto_.remainingTime();
    if (remainingMs < 0)
        remainingMs = 0;

    const int totalSeconds = remainingMs / 1000;
    ui_->te_nshot_->setTime(QTime((totalSeconds / 3600) % 24,
                                  (totalSeconds % 3600) / 60,
                                  totalSeconds % 60));
}

void MainWindow::startTargetWatchdogIfNeeded()
{
    if (controller_.settings().captureTarget.isWindow())
        t_capture_target_watchdog_.start();
}

void MainWindow::stopTargetWatchdog()
{
    t_capture_target_watchdog_.stop();
}

void MainWindow::checkTargetWatchdog()
{
    if (!isAutomaticCaptureActive())
        return;

    if (!controller_.settings().captureTarget.isWindow())
        return;

    if (controller_.refreshWindowTargetIfNeeded())
    {
        updateCaptureTargetUi();
        controller_.save();
        return;
    }

    stopAutomaticCapture(true);

    {
        QSignalBlocker blocker(ui_->cb_back_);
        ui_->cb_back_->setChecked(false);
    }

    updateAutoCaptureCheckText();

    controller_.fallbackToPrimaryScreen();
    updateCaptureTargetUi();
    updateRoiUi();
    controller_.save();

    QMessageBox::warning(this,
                         tr("Capture Target"),
                         tr("The selected capture window is no longer available.\n\n"
                            "Capture target has been changed to the primary screen.\n"
                            "Automatic capture has been stopped. Please configure the ROI again."));
}

bool MainWindow::takeShot(bool manual, const QString &marker, bool askManualComment)
{
    saveSettings();

    QStringList missing;
    if (!controller_.hasValidBaseConfiguration(&missing))
    {
        QMessageBox::warning(this,
                             manual ? tr("Manual Capture Error") : tr("Automatic Capture Error"),
                             tr("Missing or invalid configuration: %1.").arg(missing.join(", ")));
        return false;
    }

    QEventLoop loop;
    ui_->pb_take_->setDisabled(true);

    if (ui_->cb_hide_->isChecked())
    {
        hide();
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();
    }

    const auto capture = controller_.captureCurrentTarget();

    ui_->pb_take_->setDisabled(false);
    if (ui_->cb_hide_->isChecked())
        show();

    if (capture.full.isNull() || capture.crop.isNull())
    {
        QMessageBox::warning(this,
                             tr("Capture Error"),
                             capture.errorMessage.isEmpty()
                                 ? tr("The capture could not be generated.")
                                 : capture.errorMessage);
        return false;
    }

    if (manual)
    {
        manual_shot_full_ = capture.full;
        manual_shot_crop_ = capture.crop;
        updateManualScreenshotLabel();

        const QString comment = askManualComment
                                    ? requestOptionalComment(tr("Manual Capture Comment"),
                                                             tr("Optional comment for the manual capture filename:"))
                                    : marker;

        sbpshots::core::SavedShotPaths paths;
        QString error;
        if (!controller_.saveCampaignShotPair(sbpshots::core::ShotSource::Manual,
                                              comment,
                                              manual_shot_full_,
                                              manual_shot_crop_,
                                              &paths,
                                              &error))
        {
            QMessageBox::warning(this, tr("Save Error"), error);
            return false;
        }
    }
    else
    {
        auto_shot_full_ = capture.full;
        auto_shot_crop_ = capture.crop;
        updateBackupScreenshotLabel();

        sbpshots::core::SavedShotPaths paths;
        QString error;
        if (!controller_.saveCampaignShotPair(sbpshots::core::ShotSource::Auto,
                                              marker,
                                              auto_shot_full_,
                                              auto_shot_crop_,
                                              &paths,
                                              &error))
        {
            QMessageBox::warning(this, tr("Save Error"), error);
            return false;
        }
    }

    return true;
}

bool MainWindow::saveAutoMarkerShot(const QString &marker)
{
    return takeShot(false, marker, false);
}

void MainWindow::saveAuto()
{
    takeShot(false, automaticMarkerForNextShot(), false);
}

void MainWindow::saveManual()
{
    takeShot(true);
}

void MainWindow::saveIndependentShot()
{
    saveSettings();

    if (!controller_.hasValidRoi())
    {
        QMessageBox::warning(this, tr("Ad-hoc Capture"), tr("Configure the ROI before taking an ad-hoc capture."));
        return;
    }

    QEventLoop loop;

    if (ui_->cb_hide_->isChecked())
    {
        hide();
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();
    }

    const auto capture = controller_.captureCurrentTarget();

    if (ui_->cb_hide_->isChecked())
        show();

    if (capture.full.isNull() || capture.crop.isNull())
    {
        QMessageBox::warning(this,
                             tr("Ad-hoc Capture"),
                             capture.errorMessage.isEmpty()
                                 ? tr("The ad-hoc capture could not be generated.")
                                 : capture.errorMessage);
        return;
    }

    const QString folder = QFileDialog::getExistingDirectory(this,
                                                             tr("Select ad-hoc capture output folder"),
                                                             controller_.settings().basePath);
    if (folder.isEmpty())
        return;

    const QString comment = requestOptionalComment(tr("Ad-hoc Capture Comment"),
                                                   tr("Optional comment for the ad-hoc capture filename:"));

    sbpshots::core::SavedShotPaths paths;
    QString error;
    if (!controller_.saveIndependentShotPair(folder,
                                             comment,
                                             capture.full,
                                             capture.crop,
                                             &paths,
                                             &error))
    {
        QMessageBox::warning(this, tr("Save Error"), error);
        return;
    }

    QMessageBox::information(this,
                             tr("Ad-hoc Capture"),
                             tr("Ad-hoc capture saved.\n\nFull:\n%1\n\nCrop:\n%2")
                                 .arg(QDir::toNativeSeparators(paths.fullPath),
                                      QDir::toNativeSeparators(paths.cropPath)));
}

void MainWindow::updateManualScreenshotLabel()
{
    if (preview_image_kind_ == PreviewImageKind::ManualFull
        || preview_image_kind_ == PreviewImageKind::ManualCrop)
    {
        updatePreviewImageLabel();
    }
}

void MainWindow::updateBackupScreenshotLabel()
{
    if (preview_image_kind_ == PreviewImageKind::AutoFull
        || preview_image_kind_ == PreviewImageKind::AutoCrop)
    {
        updatePreviewImageLabel();
    }

    if (!auto_shot_full_.isNull())
    {
        ui_->l_backup_->setPixmap(auto_shot_full_.scaled(ui_->l_backup_->size(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation));
    }
}

void MainWindow::showLastManualCrop()
{
    ui_->rb_open_lm_crop_->setChecked(true);
    setPreviewImage(PreviewImageKind::ManualCrop);
}

void MainWindow::showLastAutoCrop()
{
    ui_->rb_open_la_crop_->setChecked(true);
    setPreviewImage(PreviewImageKind::AutoCrop);
}

void MainWindow::showLastManualFull()
{
    ui_->rb_open_lm_full_->setChecked(true);
    setPreviewImage(PreviewImageKind::ManualFull);
}

void MainWindow::showLastAutoFull()
{
    ui_->rb_open_la_full_->setChecked(true);
    setPreviewImage(PreviewImageKind::AutoFull);
}

void MainWindow::setPreviewImage(PreviewImageKind kind)
{
    preview_image_kind_ = kind;
    updatePreviewImageLabel();
}

QPixmap MainWindow::currentPreviewPixmap() const
{
    switch (preview_image_kind_)
    {
    case PreviewImageKind::ManualFull:
        return manual_shot_full_;
    case PreviewImageKind::ManualCrop:
        return manual_shot_crop_;
    case PreviewImageKind::AutoFull:
        return auto_shot_full_;
    case PreviewImageKind::AutoCrop:
        return auto_shot_crop_;
    }

    return QPixmap();
}

void MainWindow::updatePreviewImageLabel()
{
    const QPixmap pixmap = currentPreviewPixmap();

    if (pixmap.isNull())
    {
        ui_->l_screen_->clear();
        return;
    }

    ui_->l_screen_->setPixmap(pixmap.scaled(ui_->l_screen_->size(),
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));
}

void MainWindow::goToNextSection()
{
    changeSection(true);
}

void MainWindow::goToPreviousSection()
{
    changeSection(false);
}

void MainWindow::changeSection(bool forward)
{
    const bool wasAutoActive = isAutomaticCaptureActive();

    if (wasAutoActive)
    {
        const auto reply = QMessageBox::question(
            this,
            tr("Section Change"),
            tr("Automatic capture is active.\n\n"
               "The application will pause automatic capture, end the current section, "
               "change section, and resume automatic capture.\n\n"
               "Continue?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);

        if (reply != QMessageBox::Yes)
            return;

        stopAutomaticCapture(false);

        if (!ensureEndMarkerShotForCurrentSection(false))
        {
            startPeriodicAuto();
            return;
        }
    }

    if (forward)
        controller_.nextSection();
    else
        controller_.previousSection();

    controller_.settings().sequence.suffix = QStringLiteral("A");
    controller_.normalizeSequence();

    syncUiFromController();
    controller_.save();
    updateConfigurationErrors();

    if (wasAutoActive)
        startPeriodicAuto();
}

void MainWindow::goToNextVersion()
{
    saveSettings();

    controller_.settings().sequence.suffix =
        nextVersionSuffix(controller_.settings().sequence.suffix);

    controller_.normalizeSequence();

    syncUiFromController();
    updateConfigurationErrors();

    controller_.save();
}

QString MainWindow::nextVersionSuffix(const QString &suffix) const
{
    QString value = suffix.trimmed().toUpper();

    static const QRegularExpression nonLettersRegex(QStringLiteral("[^A-Z]"));
    value.remove(nonLettersRegex);

    if (value.isEmpty())
        return QStringLiteral("A");

    int index = value.size() - 1;

    while (index >= 0 && value[index] == QLatin1Char('Z'))
    {
        value[index] = QLatin1Char('A');
        --index;
    }

    if (index < 0)
        value.prepend(QLatin1Char('A'));
    else
        value[index] = QChar(value[index].unicode() + 1U);

    return value;
}

void MainWindow::forceNextAutoShot()
{
    if (!isAutomaticCaptureActive())
        return;

    if (!takeShot(false, automaticMarkerForNextShot(), false))
        return;

    t_auto_.stop();
    t_auto_.setInterval(qMax(1, ui_->sb_int_->value()) * 60000);
    t_auto_.start();

    t_next_shot_.start();

    updateNextShotCountdown();
    updateAutoActionButtons();
    updateAutoCaptureCheckText();
}

QString MainWindow::currentSectionIdToken() const
{
    const QString currentText = controller_.currentSectionText();
    const int separator = currentText.indexOf(QStringLiteral(" - "));

    const QString sectionId = separator >= 0
                                  ? currentText.left(separator)
                                  : currentText.section(QLatin1Char(' '), 0, 0);

    return sbpshots::core::PathTokens::requiredPathToken(sectionId);
}

QDir MainWindow::currentAutoShotDir(const QString &frameType) const
{
    const auto &settings = controller_.settings();

    const QString campaign = sbpshots::core::PathTokens::requiredPathToken(settings.campaign);
    const QString section = currentSectionIdToken();

    QString path = QDir(settings.basePath).filePath(campaign);
    path = QDir(path).filePath(section);
    path = QDir(path).filePath(QStringLiteral("auto"));
    path = QDir(path).filePath(frameType);

    return QDir(path);
}

bool MainWindow::directoryContainsImages(const QDir &dir) const
{
    if (!dir.exists())
        return false;

    const QStringList filters = {
        QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"),
        QStringLiteral("*.png"),
        QStringLiteral("*.bmp"),
        QStringLiteral("*.JPG"),
        QStringLiteral("*.JPEG"),
        QStringLiteral("*.PNG"),
        QStringLiteral("*.BMP")
    };

    return !dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot).isEmpty();
}

bool MainWindow::currentAutoFolderIsEmpty() const
{
    return !directoryContainsImages(currentAutoShotDir(QStringLiteral("full")))
    && !directoryContainsImages(currentAutoShotDir(QStringLiteral("crop")));
}

bool MainWindow::currentAutoMarkerExists(const QString &marker) const
{
    const QString markerToken = sbpshots::core::PathTokens::markerToken(marker).toLower();

    if (markerToken.isEmpty())
        return false;

    const QStringList filters = {
        QStringLiteral("*_%1.jpg").arg(markerToken),
        QStringLiteral("*_%1.jpeg").arg(markerToken),
        QStringLiteral("*_%1.png").arg(markerToken),
        QStringLiteral("*_%1.JPG").arg(markerToken),
        QStringLiteral("*_%1.JPEG").arg(markerToken),
        QStringLiteral("*_%1.PNG").arg(markerToken)
    };

    const QDir fullDir = currentAutoShotDir(QStringLiteral("full"));
    const QDir cropDir = currentAutoShotDir(QStringLiteral("crop"));

    if (fullDir.exists()
        && !fullDir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot).isEmpty())
    {
        return true;
    }

    if (cropDir.exists()
        && !cropDir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot).isEmpty())
    {
        return true;
    }

    return false;
}

QString MainWindow::automaticMarkerForNextShot() const
{
    if (currentAutoFolderIsEmpty())
        return QStringLiteral("start");

    return QString();
}

bool MainWindow::ensureEndMarkerShotForCurrentSection(bool askIfMissing)
{
    if (currentAutoMarkerExists(QStringLiteral("end")))
        return true;

    if (askIfMissing)
    {
        const auto reply = QMessageBox::question(
            this,
            tr("Automatic Capture"),
            tr("No automatic \"end\" capture exists for the current section.\n\n"
               "Do you want to create it now?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);

        if (reply != QMessageBox::Yes)
            return true;
    }

    return saveAutoMarkerShot(QStringLiteral("end"));
}

void MainWindow::updateWarningControlsState()
{
    /*
     * The warning period can only be edited while Alerts are disabled.
     * This avoids changing the active watchdog period while it is already armed.
     */
    ui_->sb_warn_->setEnabled(!ui_->cb_warnings_->isChecked());
}

bool MainWindow::shouldCaptureWarningTimerRun() const
{
    /*
     * Warning logic:
     *
     * - Alerts OFF: no timer.
     * - Watchdog value <= 0: no timer.
     * - Auto ON: no warning timer.
     * - Auto OFF: warning timer armed.
     */
    return ui_->cb_warnings_->isChecked()
           && ui_->sb_warn_->value() > 0
           && !isAutomaticCaptureActive();
}

void MainWindow::resetCaptureWarningCountdown()
{
    capture_warning_countdown_start_utc_ = QDateTime::currentDateTimeUtc();
}

void MainWindow::closeCaptureWarningPopup()
{
    if (!capture_warning_popup_)
        return;

    capture_warning_popup_->close();
    capture_warning_popup_->deleteLater();
    capture_warning_popup_.clear();
}

void MainWindow::startCaptureWarningTimerIfNeeded()
{
    if (shouldCaptureWarningTimerRun())
    {
        if (!capture_warning_countdown_start_utc_.isValid())
            resetCaptureWarningCountdown();

        if (!t_capture_warning_.isActive())
            t_capture_warning_.start();
    }
    else
    {
        stopCaptureWarningTimer();
    }
}

void MainWindow::stopCaptureWarningTimer()
{
    t_capture_warning_.stop();
}

void MainWindow::updateCaptureWarningTimer()
{
    if (!shouldCaptureWarningTimerRun())
    {
        stopCaptureWarningTimer();

        if (isAutomaticCaptureActive())
            closeCaptureWarningPopup();

        return;
    }

    startCaptureWarningTimerIfNeeded();
}

void MainWindow::checkCaptureWarning()
{
    if (!shouldCaptureWarningTimerRun())
    {
        updateCaptureWarningTimer();
        return;
    }

    if (!capture_warning_countdown_start_utc_.isValid())
        resetCaptureWarningCountdown();

    const QDateTime now = QDateTime::currentDateTimeUtc();

    const qint64 elapsedSeconds =
        capture_warning_countdown_start_utc_.secsTo(now);

    const qint64 warningSeconds =
        static_cast<qint64>(ui_->sb_warn_->value()) * 60;

    if (elapsedSeconds < warningSeconds)
        return;

    /*
     * Reset the countdown each time a warning is emitted.
     * As long as Auto remains OFF, a new warning will be shown periodically.
     */
    resetCaptureWarningCountdown();

    /*
     * If a previous warning popup is still open, close it and show the new one.
     */
    closeCaptureWarningPopup();
    showCaptureWarningPopup(elapsedSeconds);
}

void MainWindow::showCaptureWarningPopup(qint64 elapsedSeconds)
{
    const qint64 elapsedMinutes = qMax<qint64>(1, elapsedSeconds / 60);

    auto *messageBox = new QMessageBox(this);

    capture_warning_popup_ = messageBox;

    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->setWindowTitle(tr("Capture Watchdog"));
    messageBox->setIcon(QMessageBox::Warning);

    messageBox->setText(tr("Automatic capture is OFF."));

    messageBox->setInformativeText(
        tr("No automatic capture has been active for %1 minutes.\n\n"
           "Enable Auto Capture or disable Alerts if this is intentional.")
            .arg(elapsedMinutes));

    messageBox->setStandardButtons(QMessageBox::Ok);
    messageBox->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    connect(messageBox,
            &QObject::destroyed,
            this,
            [this, messageBox]()
            {
                if (capture_warning_popup_ == messageBox)
                    capture_warning_popup_.clear();
            });

    messageBox->show();
    messageBox->raise();
    messageBox->activateWindow();
}

QString MainWindow::makeFreeSnipToken(const QString &text) const
{
    return sbpshots::core::PathTokens::markerToken(text);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    updatePreviewImageLabel();

    if (!auto_shot_full_.isNull())
    {
        ui_->l_backup_->setPixmap(auto_shot_full_.scaled(ui_->l_backup_->size(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation));
    }
}

QPushButton *MainWindow::independentShotButton() const
{
    return findChild<QPushButton *>(QStringLiteral("pb_independent_shot_"));
}
