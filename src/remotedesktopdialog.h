/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_REMOTEDESKTOP_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_REMOTEDESKTOP_DIALOG_H

#include "quickdialog.h"
#include "remotedesktop.h"

namespace Ui
{
class RemoteDesktopDialog;
}

class RemoteDesktopDialog : public QuickDialog
{
    Q_OBJECT
public:
    RemoteDesktopDialog(const QString &appName,
                        RemoteDesktopPortal::DeviceTypes deviceTypes,
                        bool screenSharingEnabled = false,
                        bool multiple = false,
                        QObject *parent = nullptr);

    QList<quint32> selectedScreens() const;
    RemoteDesktopPortal::DeviceTypes deviceTypes() const;
};

#endif // XDG_DESKTOP_PORTAL_KDE_REMOTEDESKTOP_DIALOG_H
