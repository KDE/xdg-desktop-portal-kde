/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#pragma once

#include <QDBusArgument>
#include <QDBusMetaType>

// A generic pair type. The first element is a string that describes the type of the second element.
// This was previously called PortalIcon but is in fact used for other things as well (e.g. sounds in notifications).
struct PortalPair {
    QString str;
    QDBusVariant data;

    static auto registerDBusType()
    {
        return qDBusRegisterMetaType<PortalPair>();
    }
};

QDBusArgument &operator<<(QDBusArgument &argument, const PortalPair &icon);
const QDBusArgument &operator>>(const QDBusArgument &argument, PortalPair &icon);

Q_DECLARE_METATYPE(PortalPair)

using PortalIcon = PortalPair; // legacy compatibility
