/*
 * Copyright © 2016 Red Hat, Inc
 *    QString acceptLabel;
    bool modalDialog = true;
    bool multipleFiles = false;

    // TODO parse options - filters, choices

    if (options.contains(QLatin1String("multiple"))) {
        multipleFiles = options.value(QLatin1String("multiple")).toBool();
    }

    if (options.contains(QLatin1String("modal"))) {
        modalDialog = options.value(QLatin1String("modal")).toBool();
    }

    if (options.contains(QLatin1String("accept_label"))) {
        acceptLabel = options.value(QLatin1String("accept_label")).toString();
    }
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

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAppChooser, "xdg-desktop-portal-kde-app-chooser")


AppChooser::AppChooser(QObject *parent)
    : QObject(parent)
{
}

AppChooser::~AppChooser()
{
}

uint AppChooser::ChooseApplication(const QDBusObjectPath& handle,
                                   const QString& app_id,
                                   const QString& parent_window,
                                   const QStringList& choices,
                                   const QVariantMap& options,
                                   QVariantMap& results)
{
    qCDebug(XdgDesktopPortalKdeAppChooser) << "ChooseApplication called with parameters:";
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    choices: " << choices;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    options: " << options;

    QString acceptLabel;
    QString heading;
    QString latestChoice;
    QString title;

    if (options.contains(QLatin1String("accept_label"))) {
        acceptLabel = options.value(QLatin1String("accept_label")).toString();
    }

    if (options.contains(QLatin1String("heading"))) {
        heading = options.value(QLatin1String("heading")).toBool();
    }

    if (options.contains(QLatin1String("latest_choice"))) {
        latestChoice = options.value(QLatin1String("latest_choice")).toString();
    }

    if (options.contains(QLatin1String("title"))) {
        title = options.value(QLatin1String("title")).toString();
    }

    // TODO implement heading

    AppChooserDialog *appDialog = new AppChooserDialog(choices);
    appDialog->setLabelText(acceptLabel.isEmpty() ? QLatin1String("Select") : acceptLabel);
    appDialog->setWindowTitle(title.isEmpty() ? QLatin1String("Select application") : title);

    if (!latestChoice.isEmpty()) {
        appDialog->setSelectedApplication(latestChoice);
    }

    if (appDialog->exec()) {
        results.insert(QLatin1String("choice"), appDialog->selectedApplication());
        return 0;
    }

    return 1;
}
