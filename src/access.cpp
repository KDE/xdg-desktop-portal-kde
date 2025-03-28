/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "access.h"
#include "access_debug.h"
#include "accessdialog.h"
#include "dbushelpers.h"
#include "request.h"
#include "utils.h"

#include <QDBusConnection>
#include <QWindow>

using namespace Qt::StringLiterals;

AccessPortal::AccessPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void AccessPortal::AccessDialog(const QDBusObjectPath &handle,
                                const QString &app_id,
                                const QString &parent_window,
                                const QString &title,
                                const QString &subtitle,
                                const QString &body,
                                const QVariantMap &options,
                                const QDBusMessage &message,
                                [[maybe_unused]] uint &replyResponse,
                                [[maybe_unused]] QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeAccess) << "AccessDialog called with parameters:";
    qCDebug(XdgDesktopPortalKdeAccess) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAccess) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAccess) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAccess) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdeAccess) << "    subtitle: " << subtitle;
    qCDebug(XdgDesktopPortalKdeAccess) << "    body: " << body;
    qCDebug(XdgDesktopPortalKdeAccess) << "    options: " << options;

    auto accessDialog = new ::AccessDialog();
    accessDialog->setBody(body);
    accessDialog->setTitle(title);
    accessDialog->setSubtitle(subtitle);

    if (options.contains(QStringLiteral("deny_label"))) {
        accessDialog->setRejectLabel(options.value(QStringLiteral("deny_label")).toString());
    }

    if (options.contains(QStringLiteral("grant_label"))) {
        accessDialog->setAcceptLabel(options.value(QStringLiteral("grant_label")).toString());
    }

    if (options.contains(QStringLiteral("icon"))) {
        accessDialog->setIcon(options.value(QStringLiteral("icon")).toString());
    }

    if (options.contains(u"choices"_s)) {
        accessDialog->setChoices(qdbus_cast<OptionList>(options.value(u"choices"_s)));
    }

    accessDialog->createDialog();
    Utils::setParentWindow(accessDialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequest(handle, accessDialog);
    accessDialog->windowHandle()->setModality(options.value(QStringLiteral("modal"), true).toBool() ? Qt::WindowModal : Qt::NonModal);

    delayReply(message, accessDialog, this, [accessDialog](DialogResult result) {
        auto choices = accessDialog->selectedChoices();
        QVariantMap results{{u"choices"_s, QVariant::fromValue(choices)}};
        return QVariantList{qToUnderlying(result), results};
    });
}
