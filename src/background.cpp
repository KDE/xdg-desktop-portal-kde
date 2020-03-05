/*
 * Copyright © 2020 Red Hat, Inc
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

#include "background.h"
#include "utils.h"

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QSettings>

#include <KConfigGroup>
#include <KDesktopFile>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeBackground, "xdp-kde-background")

static QString quoteCommandPart(const QString &part)
{
    const QChar c = part.at(0);

    if (!c.isNumber() &&
        !(c == QLatin1Char('-') || c == QLatin1Char('/') || c == QLatin1Char('~') ||
          c == QLatin1Char(':') || c == QLatin1Char('.') || c == QLatin1Char('_') ||
          c == QLatin1Char('=') || c == QLatin1Char('@'))) {
        return QStringLiteral("'%1'").arg(part);
    }

    return part;
}

static QString constructCommand(const QStringList &commandline)
{
    QString command;

    for (const QString &part : commandline) {
        if (commandline.first() != part) {
            command += QLatin1Char(' ');
        }
        command += quoteCommandPart(part);
    }

    return command;
}

BackgroundPortal::BackgroundPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

BackgroundPortal::~BackgroundPortal()
{
}

QVariantMap BackgroundPortal::GetAppState()
{
    qCDebug(XdgDesktopPortalKdeBackground) << "GetAppState called: no parameters";
    return QVariantMap();
}

uint BackgroundPortal::NotifyBackground(const QDBusObjectPath &handle,
                                        const QString &app_id,
                                        const QString &name,
                                        QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeBackground) << "NotifyBackground called with parameters:";
    qCDebug(XdgDesktopPortalKdeBackground) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeBackground) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeBackground) << "    name: " << name;
    return 0;
}

bool BackgroundPortal::EnableAutostart(const QString &app_id,
                                       bool enable,
                                       const QStringList &commandline,
                                       uint flags)
{
    qCDebug(XdgDesktopPortalKdeBackground) << "EnableAutostart called with parameters:";
    qCDebug(XdgDesktopPortalKdeBackground) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeBackground) << "    enable: " << enable;
    qCDebug(XdgDesktopPortalKdeBackground) << "    commandline: " << commandline;
    qCDebug(XdgDesktopPortalKdeBackground) << "    flags: " << flags;

    const QString fileName = app_id + QStringLiteral(".desktop");
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart/");
    const QString fullPath = directory + fileName;
    const AutostartFlags autostartFlags = static_cast<AutostartFlags>(flags);

    if (!enable) {
        QFile file(fullPath);
        if (!file.remove()) {
            qCDebug(XdgDesktopPortalKdeBackground) << "Failed to remove " << fileName << " to disable autostart.";
        }
        return false;
    }

    QDir dir(directory);
    if (!dir.mkpath(dir.absolutePath())) {
        qCDebug(XdgDesktopPortalKdeBackground) << "Failed to create autostart directory.";
        return false;
    }

    KDesktopFile desktopFile(fullPath);
    KConfigGroup desktopEntryConfigGroup = desktopFile.desktopGroup();
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Type"), QStringLiteral("Application"));
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Name"), app_id);
    desktopEntryConfigGroup.writeEntry(QStringLiteral("Exec"), constructCommand(commandline));
    if (autostartFlags.testFlag(AutostartFlag::Activatable)) {
       desktopEntryConfigGroup.writeEntry(QStringLiteral("DBusActivatable"), true);
    }
    desktopEntryConfigGroup.writeEntry(QStringLiteral("X-Flatpak"), app_id);

    return true;
}

