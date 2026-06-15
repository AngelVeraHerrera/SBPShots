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
 * @file    padded_spin_box.cpp
 * @brief   QSpinBox subclass that displays integer values with configurable leading-zero padding.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// TOPASSHOTS INCLUDES
#include "TopasShots/widgets/padded_spin_box.h"

// =====================================================================================================================

PaddedSpinBox::PaddedSpinBox(QWidget *parent):
    QSpinBox(parent),
    padding_width_(3)
{
}

void PaddedSpinBox::setPaddingWidth(int width)
{
    if (width < 1)
        width = 1;

    this->padding_width_ = width;
    this->update();
}

int PaddedSpinBox::paddingWidth() const
{
    return this->padding_width_;
}

QString PaddedSpinBox::textFromValue(int value) const
{
    return QString::number(value).rightJustified(this->padding_width_, '0');
}

int PaddedSpinBox::valueFromText(const QString &text) const
{
    return text.toInt();
}
