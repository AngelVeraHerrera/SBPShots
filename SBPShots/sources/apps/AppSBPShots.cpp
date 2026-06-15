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
 * @file    AppSBPShots.cpp
 * @brief   Application entry point. Initialises Qt, applies the dark Fusion theme, and opens the main window.
 * @author  Angel Vera Herrera <avera@roa.es>
 * @date    2022-2026
***********************************************************************************************************************/

// SBPSHOTS INCLUDES
#include "SBPShots/ui/main_window.h"

// QT INCLUDES
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QPoint>
#include <QScreen>
#include <QStyle>
#include <QStyleFactory>
#include <QIcon>

// =====================================================================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/icon.ico")));

    QCoreApplication::setApplicationName("SBPShots");
    QCoreApplication::setApplicationVersion("2606.3");
    QCoreApplication::setOrganizationName("ROA");

    if (QStyle *style = QStyleFactory::create("Fusion"))
        QApplication::setStyle(style);

    QPalette palette;
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::ButtonText, Qt::white);
    QApplication::setPalette(palette);
    QApplication::setFont(QFont("Open Sans", 8, QFont::Normal));

    MainWindow window;
    window.show();

    if (const QScreen *screen = window.screen() ? window.screen() : QGuiApplication::primaryScreen())
    {
        constexpr int margin = 8;
        const QRect available = screen->availableGeometry();
        const QRect frame = window.frameGeometry();
        const QPoint topLeft(available.right() - frame.width() - margin + 1,
                             available.bottom() - frame.height() - margin + 1);
        window.move(topLeft);
    }

    return app.exec();
}
