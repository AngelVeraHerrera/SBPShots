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
 * @file    roi_selector.h
 * @brief   Full-screen transparent widget for interactive region-of-interest selection.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QPoint>
#include <QRect>
#include <QWidget>

class QScreen;

// =====================================================================================================================

/**
 * @brief Full-screen transparent overlay widget for interactive ROI selection.
 *
 * Covers an entire screen (or a custom geometry) with a semi-transparent overlay.
 * The user drags to select a rectangle; the widget then emits roiSelected() or
 * roiCanceled() and closes itself.
 */
class RoiSelector : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Constructs an ROI selector that covers the given screen.
     * @param screen               Screen to cover (uses screen geometry as selector geometry).
     * @param selectionAreaGlobal  Optional global rect that restricts where selection is allowed.
     * @param parent               Optional parent widget.
     * @param grabInput            Whether to grab keyboard and mouse input on show.
     */
    explicit RoiSelector(QScreen *screen,
                         const QRect &selectionAreaGlobal = QRect(),
                         QWidget *parent = nullptr,
                         bool grabInput = true);

    /**
     * @brief Constructs an ROI selector with an explicit global geometry.
     * @param selectorGeometryGlobal  Global geometry of the overlay widget.
     * @param selectionAreaGlobal     Optional global rect that restricts where selection is allowed.
     * @param parent                  Optional parent widget.
     * @param grabInput               Whether to grab keyboard and mouse input on show.
     */
    explicit RoiSelector(const QRect &selectorGeometryGlobal,
                         const QRect &selectionAreaGlobal = QRect(),
                         QWidget *parent = nullptr,
                         bool grabInput = true);

signals:

    /**
     * @brief Emitted when the user finishes a valid selection.
     * @param globalRect Selected rectangle in global screen coordinates.
     */
    void roiSelected(const QRect &globalRect);

    /** @brief Emitted when the user cancels the selection (Escape key or trivial selection). */
    void roiCanceled();

protected:

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:

    QPoint clampPointToSelectionArea(const QPoint &point) const;
    QRect selectedRect() const;
    QRect allowedSelectionRect() const;

    QPoint origin_;
    QRect selection_;
    QRect selection_area_local_;
    bool selecting_;
    bool grab_input_;
};
