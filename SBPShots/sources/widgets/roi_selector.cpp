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
 * @file    roi_selector.cpp
 * @brief   Full-screen transparent widget for interactive region-of-interest selection.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/widgets/roi_selector.h"

// QT INCLUDES
#include <QColor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScreen>

// =====================================================================================================================

RoiSelector::RoiSelector(QScreen *screen,
                         const QRect &selectionAreaGlobal,
                         QWidget *parent,
                         bool grabInput):
    RoiSelector(screen ? screen->geometry() : QRect(),
                selectionAreaGlobal,
                parent,
                grabInput)
{
}

RoiSelector::RoiSelector(const QRect &selectorGeometryGlobal,
                         const QRect &selectionAreaGlobal,
                         QWidget *parent,
                         bool grabInput):
    QWidget(parent),
    selecting_(false),
    grab_input_(grabInput)
{
    this->setWindowFlags(Qt::FramelessWindowHint |
                         Qt::Tool |
                         Qt::WindowStaysOnTopHint);

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setMouseTracking(true);
    this->setCursor(Qt::CrossCursor);

    this->setGeometry(selectorGeometryGlobal);

    if (selectionAreaGlobal.isValid() && !selectionAreaGlobal.isEmpty())
    {
        const QRect boundedArea =
            selectionAreaGlobal.intersected(selectorGeometryGlobal);

        if (boundedArea.isValid() && !boundedArea.isEmpty())
        {
            this->selection_area_local_ =
                boundedArea.translated(-selectorGeometryGlobal.topLeft());
        }
    }

    if (this->grab_input_)
    {
        this->grabKeyboard();
        this->grabMouse();
    }
}

QRect RoiSelector::allowedSelectionRect() const
{
    if (this->selection_area_local_.isValid() && !this->selection_area_local_.isEmpty())
        return this->selection_area_local_;

    return this->rect();
}

QPoint RoiSelector::clampPointToSelectionArea(const QPoint &point) const
{
    const QRect allowed = this->allowedSelectionRect();

    const int x = qBound(allowed.left(), point.x(), allowed.right());
    const int y = qBound(allowed.top(), point.y(), allowed.bottom());

    return QPoint(x, y);
}

QRect RoiSelector::selectedRect() const
{
    return this->selection_.normalized().intersected(this->allowedSelectionRect());
}

void RoiSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.fillRect(this->rect(), QColor(0, 0, 0, 120));

    const QRect allowed = this->allowedSelectionRect();

    painter.fillRect(allowed, QColor(255, 255, 255, 35));

    QPen allowedPen(QColor(255, 210, 80));
    allowedPen.setWidth(2);
    painter.setPen(allowedPen);
    painter.drawRect(allowed.adjusted(0, 0, -1, -1));

    const QRect rect = this->selectedRect();

    if (!rect.isEmpty())
    {
        painter.fillRect(rect, QColor(120, 190, 255, 55));

        QPen pen(Qt::yellow);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
    }

    painter.setPen(Qt::white);

    if (this->selection_area_local_.isValid() && !this->selection_area_local_.isEmpty())
    {
        painter.drawText(20,
                         30,
                         tr("Drag inside the highlighted target window to select ROI. Press Esc to cancel."));
    }
    else
    {
        painter.drawText(20,
                         30,
                         tr("Drag to select ROI. Press Esc to cancel."));
    }
}

void RoiSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    const QPoint pos = this->clampPointToSelectionArea(event->pos());

    if (!this->allowedSelectionRect().contains(pos))
        return;

    this->selecting_ = true;
    this->origin_ = pos;
    this->selection_ = QRect(this->origin_, QSize());

    this->update();
}

void RoiSelector::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->selecting_)
        return;

    const QPoint pos = this->clampPointToSelectionArea(event->pos());

    this->selection_ = QRect(this->origin_, pos);
    this->update();
}

void RoiSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    this->selecting_ = false;

    const QPoint pos = this->clampPointToSelectionArea(event->pos());
    this->selection_ = QRect(this->origin_, pos).normalized();

    if (this->grab_input_)
    {
        this->releaseKeyboard();
        this->releaseMouse();
    }

    const QRect finalSelection = this->selectedRect();

    if (finalSelection.width() > 5 && finalSelection.height() > 5)
    {
        const QRect globalRect = finalSelection.translated(this->geometry().topLeft());
        emit this->roiSelected(globalRect.normalized());
    }
    else
    {
        emit this->roiCanceled();
    }

    this->close();
}

void RoiSelector::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        if (this->grab_input_)
        {
            this->releaseKeyboard();
            this->releaseMouse();
        }

        emit this->roiCanceled();
        this->close();
        return;
    }

    QWidget::keyPressEvent(event);
}
