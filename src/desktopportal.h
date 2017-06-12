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

class DesktopPortal : public QObject
{
    Q_OBJECT
public:
    explicit DesktopPortal(QObject *parent = nullptr);
    ~DesktopPortal();

private:
    AccessPortal *m_access;
    AppChooserPortal *m_appChooser;
    EmailPortal *m_email;
    FileChooserPortal *m_fileChooser;
    InhibitPortal *m_inhibit;
    NotificationPortal *m_notification;
    PrintPortal *m_print;
};

#endif // XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H

