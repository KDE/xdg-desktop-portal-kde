/*
 * SPDX-FileCopyrightText: 2016-2018 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "appchooser.h"
#include "appchooserdialog.h"
#include "request.h"
#include "utils.h"

#include <QLoggingCategory>

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

    if (options.contains(QStringLiteral("last_choice"))) {
        latestChoice = options.value(QStringLiteral("last_choice")).toString();
    }

    QVariant itemName = options.value(QStringLiteral("filename"));
    if (!itemName.isValid()) {
        itemName = options.value(QStringLiteral("content_type"));
    }
    AppChooserDialog *appDialog = new AppChooserDialog(choices, latestChoice, itemName.toString(), options.value(QStringLiteral("content_type")).toString());
    m_appChooserDialogs.insert(handle.path(), appDialog);
    Utils::setParentWindow(appDialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequest(handle, appDialog);

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
