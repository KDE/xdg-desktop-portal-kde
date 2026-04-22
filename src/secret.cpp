/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
*/

#include "secret.h"

#include "request.h"

#include <KConfig>
#include <KConfigGroup>

#include <QDBusConnection>

using namespace Qt::StringLiterals;

SecretPortal::SecretPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    if (KConfig(u"???"_s, KConfig::CascadeConfig).group(u"???"_s).readEntry(u"???"_s) == u"???"_s) {
        m_implementation = u"org.freedesktop.impl.portal.desktop.oo7"_s;
    } else {
        m_implementation = u"org.freedesktop.impl.portal.desktop.kwallet"_s;
    }
}

void SecretPortal::RetrieveSecret(const QDBusObjectPath &handle,
                                  const QString &app_id,
                                  const QDBusUnixFileDescriptor &fd,
                                  const QVariantMap &options,
                                  const QDBusMessage &message,
                                  [[maybe_unused]] uint &replyResponse,
                                  [[maybe_unused]] QVariantMap &replyResults)
{
    message.setDelayedReply(true);

    auto msg =
        QDBusMessage::createMethodCall(m_implementation, u"/org/freedesktop/portal/desktop"_s, u"org.freedesktop.impl.portal.Secret"_s, u"RetrieveSecrets"_s);
    msg.setArguments({QVariant::fromValue(handle), app_id, QVariant::fromValue(fd), options});
    QDBusPendingReply<uint, QVariantMap> reply = QDBusConnection::sessionBus().asyncCall(msg);

    auto watcher = new QDBusPendingCallWatcher(reply);

    auto request = new Request(handle, watcher);
    connect(request, &Request::closeRequested, this, [this, handle] {
        auto msg = QDBusMessage::createMethodCall(m_implementation, handle.path(), u"org.freedesktop.impl.portal.Request"_s, u"Close"_s);
        QDBusConnection::sessionBus().send(msg);
    });

    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [message, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<uint, QVariantMap> result = *watcher;
        QDBusMessage reply;
        if (result.isError()) {
            reply = message.createErrorReply(result.error());
        } else {
            reply = message.createReply({result.argumentAt(0), result.argumentAt(1)});
        }
        QDBusConnection::sessionBus().send(reply);
    });
}

#include "moc_secret.cpp"
