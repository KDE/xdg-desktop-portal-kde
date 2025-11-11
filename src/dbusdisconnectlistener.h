/*
 * SPDX-FileCopyrightText: 2025 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QDBusConnection>
#include <QObject>

class DBusDisconnectListener : public QObject
{
    Q_OBJECT

public:
    explicit DBusDisconnectListener(QDBusConnection, QObject *parent = nullptr);

Q_SIGNALS:
    void disconnected();
};
