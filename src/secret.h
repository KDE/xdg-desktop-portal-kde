/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>

/*
 * Adaptor class for interface org.freedesktop.impl.portal.Secret
 */
class SecretPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Secret")
    Q_PROPERTY(uint version MEMBER version CONSTANT)
public:
    SecretPortal(QObject *parent);
    constexpr static uint version = 1;

public Q_SLOTS:
    void RetrieveSecret(const QDBusObjectPath &handle,
                        const QString &app_id,
                        const QDBusUnixFileDescriptor &fd,
                        const QVariantMap &options,
                        const QDBusMessage &message,
                        uint &replyResponse,
                        QVariantMap &replyResults);

private:
    QString m_implementation;
};
