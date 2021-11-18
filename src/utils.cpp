/*
 * SPDX-FileCopyrightText: 2018 Alexander Volkov <a.volkov@rusbitech.ru>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "utils.h"

#include <KWindowSystem>

#include <QString>
#include <QWidget>

void Utils::setParentWindow(QWidget *w, const QString &parent_window)
{
    if (parent_window.startsWith(QLatin1String("x11:"))) {
        w->setAttribute(Qt::WA_NativeWindow, true);
        setParentWindow(w->windowHandle(), parent_window);
    }
}

void Utils::setParentWindow(QWindow *w, const QString &parent_window)
{
    if (parent_window.startsWith(QLatin1String("x11:"))) {
        KWindowSystem::setMainWindow(w, parent_window.midRef(4).toULongLong(nullptr, 16));
    }
}
