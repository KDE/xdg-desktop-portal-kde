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

#include <QApplication>
#include <QDBusConnection>
#include <QLoggingCategory>

#include "desktopportal.h"

#include "appchooser.h"
#include "dbus/AppChooserAdaptor.h"

#include "filechooser.h"
#include "dbus/FileChooserAdaptor.h"

Q_LOGGING_CATEGORY(XdgDesktopPortalKde, "xdg-desktop-portal-kde")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (sessionBus.registerService(QLatin1String("org.freedesktop.impl.portal.desktop.kde"))) {
        DesktopPortal *desktopPortal = new DesktopPortal(&a);
        if (sessionBus.registerVirtualObject(QLatin1String("/org/freedesktop/portal/desktop"), desktopPortal, QDBusConnection::VirtualObjectRegisterOption::SingleNode)) {
            qCDebug(XdgDesktopPortalKde) << "Desktop portal registered successfuly";
        } else {
            qCDebug(XdgDesktopPortalKde) << "Failed to register desktop portal";
        }
    } else {
        qCDebug(XdgDesktopPortalKde) << "Failed to register org.freedesktop.impl.portal.desktop.kde service";
    }

    return a.exec();
}
