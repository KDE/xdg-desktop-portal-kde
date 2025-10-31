// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QStringList>
#include <QVariant>

class QDBusMessage;

class SaveRestore : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.SaveRestore")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    explicit SaveRestore(QObject *parent = nullptr)
        : QDBusAbstractAdaptor(parent)
    {
    }

public Q_SLOTS:
    QVariantMap Register(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options);

    void DiscardedInstanceIds(const QDBusObjectPath &session_handle, const QStringList &instance_ids)
    {
        Q_UNUSED(session_handle)
        Q_UNUSED(instance_ids)
    }

    void SaveHintResponse(const QDBusObjectPath &session_handle)
    {
        Q_UNUSED(session_handle)
    }

Q_SIGNALS:
    void SaveHint(const QDBusObjectPath &session_handle);
    void Quit(const QDBusObjectPath &session_handle);

public:
    uint version() const
    {
        return 1u;
    }

private:
    // app-id, list -objectPath
    QMultiHash<QString, QString> m_registrations;
};
