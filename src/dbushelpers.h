/*
 * SPDX-FileCopyrightText: 2018-2019 Red Hat Inc
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018-2019 Jan Grulich <jgrulich@redhat.com>
 */

#pragma once

#include <QDBusArgument>
#include <QMap>
#include <QString>

/// a{sa{sv}}
using VariantMapMap = QMap<QString, QMap<QString, QVariant>>;

/// sa{sv}
using Shortcut = QPair<QString, QVariantMap>;

/// a(sa{sv})
using Shortcuts = QList<Shortcut>;

// as
using Permissions = QStringList;

// a{sas}
using AppIdPermissionsMap = QMap<QString, Permissions>;

Q_DECLARE_METATYPE(VariantMapMap)
Q_DECLARE_METATYPE(Shortcuts)

struct Choice {
    Q_GADGET
public:
    QString id;
    QString value;
};
using Choices = QList<Choice>;
QDBusArgument &operator<<(QDBusArgument &arg, const Choice &choice);
const QDBusArgument &operator>>(const QDBusArgument &arg, Choice &choice);

struct Option {
    Q_GADGET
public:
    QString id;
    QString label;
    Choices choices;
    QString initialChoiceId;
};
using OptionList = QList<Option>;
QDBusArgument &operator<<(QDBusArgument &arg, const Option &option);
const QDBusArgument &operator>>(const QDBusArgument &arg, Option &option);
