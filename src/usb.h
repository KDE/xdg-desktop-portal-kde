/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include <tuple>

class QDBusMessage;

class UsbPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Usb")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    explicit UsbPortal(QObject *parent);

    static constexpr uint version()
    {
        return 1;
    }
public Q_SLOTS:
    void AcquireDevices(const QDBusObjectPath &handle,
                        const QString &parent_window,
                        const QString &app_id,
                        const QList<std::tuple<QString, QVariantMap, QVariantMap>> &devices,
                        const QVariantMap &options,
                        const QDBusMessage &message,
                        uint &replyResponse,
                        QVariantMap &replyResults);
};
