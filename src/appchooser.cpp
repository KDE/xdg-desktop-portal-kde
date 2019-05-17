/*
 * Copyright © 2016-2018 Red Hat, Inc
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

#include "appchooser.h"
#include "appchooserdialog.h"
#include "utils.h"

#include <QLoggingCategory>

#include <KLocalizedString>
#include <KMimeTypeTrader>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAppChooser, "xdp-kde-app-chooser")

/**
* Taken from KServiceUtilPrivate, it's used to workaround KService returning desktopEntryName()
* as lowercase string
*
* Lightweight implementation of QFileInfo::completeBaseName.
*
* Returns the complete base name of the file without the path.
* The complete base name consists of all characters in the file up to (but not including) the last '.' character.
*
* Example: "/tmp/archive.tar.gz" --> "archive.tar"
*/
static QString completeBaseName(const QString& filepath)
{
    QString name = filepath;
    int pos = name.lastIndexOf(QLatin1Char('/'));
    if (pos != -1) {
        name = name.mid(pos + 1);
    }
    pos = name.lastIndexOf(QLatin1Char('.'));
    if (pos != -1) {
        name.truncate(pos);
    }
    return name;
}

AppChooserPortal::AppChooserPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

AppChooserPortal::~AppChooserPortal()
{
}

uint AppChooserPortal::ChooseApplication(const QDBusObjectPath &handle,
                                         const QString &app_id,
                                         const QString &parent_window,
                                         const QStringList &choices,
                                         const QVariantMap &options,
                                         QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeAppChooser) << "ChooseApplication called with parameters:";
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    choices: " << choices;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    options: " << options;

    QString latestChoice;

    if (options.contains(QStringLiteral("last_choice"))) {
        latestChoice = options.value(QStringLiteral("last_choice")).toString();
    }

    if (options.contains(QStringLiteral("use_associated_app"))) {
        const bool useAssociatedApp = options.value(QStringLiteral("use_associated_app")).toBool();

        if (useAssociatedApp && options.contains(QStringLiteral("content_type"))) {
            const QString contentType = options.value(QStringLiteral("content_type")).toString();

            KService::Ptr service = KMimeTypeTrader::self()->preferredService(contentType);
            if (service->isValid()) {
                results.insert(QStringLiteral("choice"), completeBaseName(service->entryPath()));
                return 0;
            }
        }
    }

    AppChooserDialog *appDialog = new AppChooserDialog(choices, latestChoice, options.value(QStringLiteral("filename")).toString());
    m_appChooserDialogs.insert(handle.path(), appDialog);
    Utils::setParentWindow(appDialog, parent_window);

    int result = appDialog->exec();

    if (result) {
        results.insert(QStringLiteral("choice"), appDialog->selectedApplication());
    }

    m_appChooserDialogs.remove(handle.path());
    appDialog->deleteLater();

    return !result;
}

void AppChooserPortal::UpdateChoices(const QDBusObjectPath &handle, const QStringList &choices)
{
    qCDebug(XdgDesktopPortalKdeAppChooser) << "UpdateChoices called with parameters:";
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    choices: " << choices;

    if (m_appChooserDialogs.contains(handle.path())) {
        m_appChooserDialogs.value(handle.path())->updateChoices(choices);
    }
}
