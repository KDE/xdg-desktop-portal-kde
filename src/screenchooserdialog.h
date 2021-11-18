/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H

#include "quickdialog.h"
#include "screencast.h"
#include <QEventLoop>

class ScreenChooserDialog : public QuickDialog
{
    Q_OBJECT
public:
    ScreenChooserDialog(const QString &appName, bool multiple, ScreenCastPortal::SourceTypes types);
    ~ScreenChooserDialog() override;

    QList<quint32> selectedScreens() const;
    QVector<QMap<int, QVariant>> selectedWindows() const;

Q_SIGNALS:
    void clearSelection();
};

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H
