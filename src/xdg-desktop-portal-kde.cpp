/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>

#include "../version.h"
#include "debug.h"
#include "desktopportal.h"

#include <signal.h>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    // Do not try to reconnect to the compositor as KWayland can't handle that
    // Reconsider enabling when we do not use KWayland anymore
    qunsetenv("QT_WAYLAND_RECONNECT");
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    a.setQuitLockEnabled(false);

    // Guard against an app closing the reading end when the clipboard portal writes current clipboard contents
    signal(SIGPIPE, SIG_IGN);

    QCommandLineParser parser;
    QCommandLineOption replaceOption(u"replace"_s, u"Replace running instance"_s);
    parser.addOption(replaceOption);

    KAboutData about(QStringLiteral("xdg-desktop-portal-kde"), QString(), QStringLiteral(XDPK_VERSION_STRING));
    about.setDesktopFileName(QStringLiteral("org.freedesktop.impl.portal.desktop.kde"));
    about.setupCommandLine(&parser);
    KAboutData::setApplicationData(about);

    parser.process(a);
    about.processCommandLine(&parser);

    KCrash::initialize();

    const auto dbusQueueOption = parser.isSet(replaceOption) ? QDBusConnectionInterface::ReplaceExistingService : QDBusConnectionInterface::DontQueueService;

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.interface()->registerService(u"org.freedesktop.impl.portal.desktop.kde"_s, dbusQueueOption, QDBusConnectionInterface::AllowReplacement)) {
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
