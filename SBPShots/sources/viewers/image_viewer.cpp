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
 * @file    image_viewer.cpp
 * @brief   Standalone image viewer window with zoom, fit-to-window, and scroll support.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/viewers/image_viewer.h"

// QT INCLUDES
#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>

// =====================================================================================================================

ImageViewer::ImageViewer(QWidget *parent):
    QMainWindow(parent),
    l_image_(new QLabel),
    sarea_(new QScrollArea),
    scale_factor_(1.0),
    zoom_in_act_(nullptr),
    zoom_out_act_(nullptr),
    normal_size_act_(nullptr),
    fit_to_window_act_(nullptr)
{
    this->l_image_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    this->l_image_->setScaledContents(true);

    this->sarea_->setWidget(this->l_image_);
    this->sarea_->setVisible(false);
    this->setCentralWidget(this->sarea_);

    this->createActions();

    if (const QScreen *screen = QGuiApplication::primaryScreen())
        this->resize(screen->availableSize() * 3 / 5);
    else
        this->resize(QSize(800, 600));
}

void ImageViewer::setPixmap(const QPixmap &pixmap)
{
    this->l_image_->setPixmap(pixmap);

    this->scale_factor_ = 0.7;

    this->sarea_->setVisible(true);
    this->fit_to_window_act_->setEnabled(true);
    this->updateActions();

    if (!this->fit_to_window_act_->isChecked())
        this->l_image_->adjustSize();
}

void ImageViewer::zoomIn()
{
    this->scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    this->scaleImage(0.8);
}

void ImageViewer::normalSize()
{
    this->l_image_->adjustSize();
    this->scale_factor_ = 1.0;
}

void ImageViewer::fitToWindow()
{
    const bool fitToWindow = this->fit_to_window_act_->isChecked();

    this->sarea_->setWidgetResizable(fitToWindow);

    if (!fitToWindow)
        this->normalSize();

    this->updateActions();
}

void ImageViewer::createActions()
{
    QMenu *viewMenu = this->menuBar()->addMenu(tr("&View"));

    this->zoom_in_act_ = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageViewer::zoomIn);
    this->zoom_in_act_->setShortcut(QKeySequence::ZoomIn);
    this->zoom_in_act_->setEnabled(false);

    this->zoom_out_act_ = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageViewer::zoomOut);
    this->zoom_out_act_->setShortcut(QKeySequence::ZoomOut);
    this->zoom_out_act_->setEnabled(false);

    this->normal_size_act_ = viewMenu->addAction(tr("&Normal Size"), this, &ImageViewer::normalSize);
    this->normal_size_act_->setShortcut(tr("Ctrl+S"));
    this->normal_size_act_->setEnabled(false);

    viewMenu->addSeparator();

    this->fit_to_window_act_ = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewer::fitToWindow);
    this->fit_to_window_act_->setEnabled(false);
    this->fit_to_window_act_->setCheckable(true);
    this->fit_to_window_act_->setShortcut(tr("Ctrl+F"));
}

void ImageViewer::updateActions()
{
    const bool fitToWindow = this->fit_to_window_act_->isChecked();

    this->zoom_in_act_->setEnabled(!fitToWindow);
    this->zoom_out_act_->setEnabled(!fitToWindow);
    this->normal_size_act_->setEnabled(!fitToWindow);
}

void ImageViewer::scaleImage(double factor)
{
    this->scale_factor_ *= factor;

    const QPixmap pixmap = this->l_image_->pixmap();

    if (pixmap.isNull())
        return;

    this->l_image_->resize(pixmap.size() * this->scale_factor_);

    this->adjustScrollBar(this->sarea_->horizontalScrollBar(), factor);
    this->adjustScrollBar(this->sarea_->verticalScrollBar(), factor);

    this->zoom_in_act_->setEnabled(this->scale_factor_ < 3.0);
    this->zoom_out_act_->setEnabled(this->scale_factor_ > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    if (!scrollBar)
        return;

    scrollBar->setValue(static_cast<int>(
        factor * scrollBar->value() + ((factor - 1.0) * scrollBar->pageStep() / 2.0)
        ));
}
