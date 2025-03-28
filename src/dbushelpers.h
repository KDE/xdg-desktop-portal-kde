/*
 * SPDX-FileCopyrightText: 2018-2019 Red Hat Inc
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018-2019 Jan Grulich <jgrulich@redhat.com>
 */

#pragma once

#include "quickdialog.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDialog>
#include <QMap>
#include <QString>

#include "utils.h"

// not an enum class so it can convert to uint implicitelly when returning from a dbus handling slot
namespace PortalResponse
{
enum Response : unsigned {
    Success = 0,
    Cancelled = 1,
    OtherError = 2
};
constexpr inline Response fromDialogResult(DialogResult r)
{
    return r == DialogResult::Accepted ? Success : Cancelled;
}
constexpr inline Response fromDialogCode(QDialog::DialogCode c)
{
    return c == QDialog::Accepted ? Success : Cancelled;
}
}

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
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(QString value MEMBER value CONSTANT)
public:
    QString id;
    QString value;
};
using Choices = QList<Choice>;
QDBusArgument &operator<<(QDBusArgument &arg, const Choice &choice);
const QDBusArgument &operator>>(const QDBusArgument &arg, Choice &choice);

struct Option {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(QString label MEMBER label CONSTANT)
    Q_PROPERTY(Choices choices MEMBER choices CONSTANT)
    Q_PROPERTY(QString initialChoiceId MEMBER initialChoiceId CONSTANT)
public:
    QString id;
    QString label;
    Choices choices;
    QString initialChoiceId;
};
using OptionList = QList<Option>;
QDBusArgument &operator<<(QDBusArgument &arg, const Option &option);
const QDBusArgument &operator>>(const QDBusArgument &arg, Option &option);

inline void delayReply(const QDBusMessage &message, auto *dialog, auto *contextObject, auto &&callback)
{
    message.setDelayedReply(true);
    QObject::connect(dialog, &std::remove_pointer_t<decltype(dialog)>::finished, contextObject, [message, callback](const auto &result) {
        const auto reply = message.createReply(std::invoke(callback, result));
        QDBusConnection::sessionBus().send(reply);
    });
}
