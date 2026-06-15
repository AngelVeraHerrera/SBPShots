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
 * @file    padded_spin_box.h
 * @brief   QSpinBox subclass that displays integer values with configurable leading-zero padding.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

#pragma once

// QT INCLUDES
#include <QSpinBox>

// =====================================================================================================================

/**
 * @brief QSpinBox that displays integer values padded with leading zeroes.
 *
 * The padding width is configurable at runtime via setPaddingWidth(). The default
 * padding width is 3, so values are displayed as "001", "002", etc.
 */
class PaddedSpinBox : public QSpinBox
{
    Q_OBJECT

public:

    explicit PaddedSpinBox(QWidget *parent = nullptr);

    /**
     * @brief Sets the minimum number of digits to display, padding with leading zeroes.
     * @param width Desired display width (clamped to >= 1).
     */
    void setPaddingWidth(int width);

    /** @brief Returns the current display padding width. */
    int paddingWidth() const;

protected:

    QString textFromValue(int value) const override;
    int valueFromText(const QString &text) const override;

private:

    int padding_width_;
};
