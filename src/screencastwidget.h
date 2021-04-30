/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENCAST_WIDGET_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENCAST_WIDGET_H

#include <QListWidget>

class ScreenCastWidget : public QListWidget
{
    Q_OBJECT
public:
    ScreenCastWidget(QWidget *parent = nullptr);
    ~ScreenCastWidget();

    QList<quint32> selectedScreens() const;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENCAST_WIDGET_H
