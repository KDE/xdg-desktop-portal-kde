/*
 * SPDX-FileCopyrightText: 2016-2018 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "appchooser.h"
#include "appchooser_debug.h"
#include "appchooserdialog.h"
#include "request.h"
#include "utils.h"

#include <KAuthorized>
#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

using namespace Qt::StringLiterals;

AppChooserPortal::AppChooserPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void AppChooserPortal::ChooseApplication(const QDBusObjectPath &handle,
                                         const QString &app_id,
                                         const QString &parent_window,
                                         const QStringList &choices,
                                         const QVariantMap &options,
                                         const QDBusMessage &message,
                                         [[maybe_unused]] uint &replyResponse,
                                         [[maybe_unused]] QVariantMap &replyResults)
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
    auto appDialog = new AppChooserDialog(choices, latestChoice, itemName.toString(), options.value(QStringLiteral("content_type")).toString(), true);
    m_appChooserDialogs.insert(handle.path(), appDialog);
    Utils::setParentWindow(appDialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequest(handle, appDialog);

    delayReply(message, appDialog, this, [this, appDialog, handle](DialogResult result) {
        QVariantMap results;
        if (result == DialogResult::Accepted) {
            results = {{QStringLiteral("choice"), appDialog->selectedApplication()}, {QStringLiteral("activation_token"), appDialog->activationToken()}};
        }
        m_appChooserDialogs.remove(handle.path());
        return QVariantList{PortalResponse::fromDialogResult(result), results};
    });
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

uint AppChooserPortal::ChooseApplicationPrivate(const QString &parent_window,
                                                const QStringList &urls,
                                                const QVariantMap &options,
                                                const QDBusMessage &msg,
                                                [[maybe_unused]] QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeAppChooser) << "ChooseApplicationPrivate called with parameters:";
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    urls: " << urls;
    qCDebug(XdgDesktopPortalKdeAppChooser) << "    options: " << options;

    if (urls.isEmpty()) {
        return 1;
    }

    const QString itemName = urls.size() == 1 ? urls.at(0) : i18nc("count of files to open", "%1 files", urls.size());

    auto appDialog = new AppChooserDialog({},
                                          options.value(QStringLiteral("last_choice")).toString(),
                                          itemName,
                                          options.value(QStringLiteral("content_type")).toString(),
                                          false);
    Utils::setParentWindow(appDialog->windowHandle(), parent_window);
    msg.setDelayedReply(true);

    appDialog->m_appChooserData->setHistory(options.value("history"_L1).toStringList());
    appDialog->m_appChooserData->setShellAccess(KAuthorized::authorize(KAuthorized::SHELL_ACCESS));

    QDBusServiceWatcher watcher(msg.service(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration);
    connect(&watcher, &QDBusServiceWatcher::serviceUnregistered, appDialog, [appDialog] {
        appDialog->reject();
    });

    delayReply(msg, appDialog, this, [appDialog](DialogResult result) {
        QVariantMap results;
        if (result == DialogResult::Accepted) {
            results.insert(QStringLiteral("choice"), appDialog->selectedApplication());
            results.insert(QStringLiteral("remember"), appDialog->m_appChooserData->m_remember);
            results.insert(QStringLiteral("openInTerminal"), appDialog->m_appChooserData->m_openInTerminal);
            results.insert(QStringLiteral("lingerTerminal"), appDialog->m_appChooserData->m_lingerTerminal);
        }
        return QVariantList{PortalResponse::fromDialogResult(result), results};
    });
    return PortalResponse::Success;
}
