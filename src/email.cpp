/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#include "email.h"
#include "email_debug.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QUrl>

#include <KEMailClientLauncherJob>

EmailPortal::EmailPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void EmailPortal::ComposeEmail(const QDBusObjectPath &handle,
                               const QString &app_id,
                               const QString &window,
                               const QVariantMap &options,
                               const QDBusMessage &message,
                               [[maybe_unused]] uint &response,
                               [[maybe_unused]] QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeEmail) << "ComposeEmail called with parameters:";
    qCDebug(XdgDesktopPortalKdeEmail) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeEmail) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeEmail) << "    window: " << window;
    qCDebug(XdgDesktopPortalKdeEmail) << "    options: " << options;

    const QStringList addresses = options.contains(QStringLiteral("address")) ? options.value(QStringLiteral("address")).toStringList()
                                                                              : options.value(QStringLiteral("addresses")).toStringList();

    auto job = new KEMailClientLauncherJob;
    job->setTo(addresses);
    job->setCc(options.value(QStringLiteral("cc")).toStringList());
    job->setBcc(options.value(QStringLiteral("bcc")).toStringList());
    job->setSubject(options.value(QStringLiteral("subject")).toString());
    job->setBody(options.value(QStringLiteral("body")).toString());

    const QStringList attachmentStrings = options.value(QStringLiteral("attachments")).toStringList();
    QList<QUrl> attachments;
    for (const QString &attachment : attachmentStrings) {
        attachments << QUrl(attachment);
    }
    job->setAttachments(attachments);

    job->start();
    connect(job, &KEMailClientLauncherJob::finished, this, [message](KJob *job) {
        auto reply = message.createReply(job->error() ? 2 : 0);
        QDBusConnection::sessionBus().send(reply);
    });
}
