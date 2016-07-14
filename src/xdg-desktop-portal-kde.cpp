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
        qCDebug(XdgDesktopPortalKde) << "Service org.freedesktop.impl.portal.desktop.kde registered successfuly";

        // AppChooser
        AppChooser *appChooser = new AppChooser(&a);
        AppChooserAdaptor *appChooserAdaptor = new AppChooserAdaptor(appChooser);
        if (sessionBus.registerObject(QLatin1String("/org/freedesktop/portal/desktop"), appChooser)) {
            qCDebug(XdgDesktopPortalKde) << "Interface AppChooser registered successfuly";
        } else {
            qCDebug(XdgDesktopPortalKde) << "Failed to register AppChooser interface";
        }

        // FileChooser
        FileChooser *fileChooser = new FileChooser(&a);
        FileChooserAdaptor *fileChooserAdaptor = new FileChooserAdaptor(fileChooser);
        if (sessionBus.registerObject(QLatin1String("/org/freedesktop/portal/desktop"), fileChooser)) {
            qCDebug(XdgDesktopPortalKde) << "Interface FileChooser registered successfuly";
        } else {
            qCDebug(XdgDesktopPortalKde) << "Failed to register FileChooser interface";
        }
    } else {
        qCDebug(XdgDesktopPortalKde) << "Failed to register org.freedesktop.impl.portal.desktop.kde service";
    }

    return a.exec();
}
