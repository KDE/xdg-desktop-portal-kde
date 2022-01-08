/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#include <QApplication>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <KAboutData>
#include <KLocalizedString>

#include "../version.h"
#include "desktopportal.h"

Q_LOGGING_CATEGORY(XdgDesktopPortalKde, "xdp-kde")

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    KAboutData about(QStringLiteral("xdg-desktop-portal-kde"), i18n("Portal"), QStringLiteral(XDPK_VERSION_STRING));
    KAboutData::setApplicationData(about);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (sessionBus.registerService(QStringLiteral("org.freedesktop.impl.portal.desktop.kde"))) {
        DesktopPortal *desktopPortal = new DesktopPortal(&a);
        if (sessionBus.registerObject(QStringLiteral("/org/freedesktop/portal/desktop"), desktopPortal, QDBusConnection::ExportAdaptors)) {
            qCDebug(XdgDesktopPortalKde) << "Desktop portal registered successfully";
        } else {
            qCDebug(XdgDesktopPortalKde) << "Failed to register desktop portal";
        }
    } else {
        qCDebug(XdgDesktopPortalKde) << "Failed to register org.freedesktop.impl.portal.desktop.kde service";
        return 1;
    }

    return a.exec();
}
