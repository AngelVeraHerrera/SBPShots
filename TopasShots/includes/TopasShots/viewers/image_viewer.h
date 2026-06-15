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
 * @file    image_viewer.h
 * @brief   Standalone image viewer window with zoom, fit-to-window, and scroll support.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QAction>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>

// =====================================================================================================================

/**
 * @brief Standalone image viewer window with zoom, normal-size, and fit-to-window controls.
 */
class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:

    explicit ImageViewer(QWidget *parent = nullptr);

    /**
     * @brief Displays the given pixmap in the viewer and resets the zoom level.
     * @param pixmap Pixmap to display.
     */
    void setPixmap(const QPixmap &pixmap);

private slots:

    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();

private:

    void createActions();
    void updateActions();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QImage image_;
    QLabel *l_image_;
    QScrollArea *sarea_;
    double scale_factor_;

    QAction *zoom_in_act_;
    QAction *zoom_out_act_;
    QAction *normal_size_act_;
    QAction *fit_to_window_act_;
};
