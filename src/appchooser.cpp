/*
 * Copyright © 2016 Red Hat, Inc
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

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAppChooser, "xdp-kde-app-chooser")


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

    if (options.contains(QLatin1String("last_choice"))) {
        latestChoice = options.value(QLatin1String("last_choice")).toString();
    }

    AppChooserDialog *appDialog = new AppChooserDialog(choices, latestChoice, options.value(QLatin1String("filename")).toString());
    Utils::setParentWindow(appDialog, parent_window);

    if (appDialog->exec()) {
        results.insert(QLatin1String("choice"), appDialog->selectedApplication());
        appDialog->deleteLater();
        return 0;
    }
    appDialog->deleteLater();

    return 1;
}
