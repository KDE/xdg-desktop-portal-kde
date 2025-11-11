/*
 * SPDX-FileCopyrightText: 2025 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "dbusdisconnectlistener.h"

DBusDisconnectListener::DBusDisconnectListener(QDBusConnection connection, QObject *parent)
    : QObject(parent)
{
    connection.connect(QString(),
                       QStringLiteral("/org/freedesktop/DBus/Local"),
                       QStringLiteral("org.freedesktop.DBus.Local"),
                       QStringLiteral("Disconnected"),
                       this,
                       SIGNAL(disconnected()));
}
