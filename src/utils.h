/*
 * SPDX-FileCopyrightText: 2018 Alexander Volkov <a.volkov@rusbitech.ru>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_UTILS_H
#define XDG_DESKTOP_PORTAL_KDE_UTILS_H

class QString;
class QWidget;

class Utils
{
public:
    static void setParentWindow(QWidget *w, const QString &parent_window);
};

#endif // XDG_DESKTOP_PORTAL_KDE_UTILS_H
