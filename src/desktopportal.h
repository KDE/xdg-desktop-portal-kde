/*
 * Copyright © 2016 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
#define XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H

#include <QObject>
#include <QDBusVirtualObject>

#include "access.h"
#include "appchooser.h"
#include "email.h"
#include "filechooser.h"
#include "inhibit.h"
#include "notification.h"
#include "print.h"

class DesktopPortal : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit DesktopPortal(QObject *parent = nullptr);
    ~DesktopPortal();

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) Q_DECL_OVERRIDE;
    QString introspect(const QString &path) const Q_DECL_OVERRIDE;
private:
    Access *m_access;
    AppChooser *m_appChooser;
    Email *m_email;
    FileChooser *m_fileChooser;
    Inhibit *m_inhibit;
    Notification *m_notification;
    Print *m_print;
};

#endif // XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H

