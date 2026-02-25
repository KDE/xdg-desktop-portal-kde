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

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDialog>
#include <QMap>
#include <QMetaMethod>
#include <QString>

#include "utils.h"

#include <source_location>

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

template<typename Function>
struct member_function_traits;
template<typename C, typename R, typename... Args>
struct member_function_traits<R (C::*)(Args...)> {
    using Class = C;
    using Return = R;
    using Arguments = std::tuple<Args...>;
};

template<typename Signal, typename... Args>
void sendSignal(Signal &&function, Args &&...args)
    requires std::is_member_function_pointer_v<Signal> && std::derived_from<typename member_function_traits<Signal>::Class, QDBusAbstractAdaptor>
    && std::invocable<Signal, typename member_function_traits<Signal>::Class, Args...>
{
    using namespace Qt::StringLiterals;
    const auto metaSignal = QMetaMethod::fromSignal(function);
    if (!metaSignal.isValid()) {
        qWarning() << "function passed is not a signal" << std::source_location::current().function_name();
        return;
    }
    const QMetaObject &metaObject = member_function_traits<Signal>::Class::staticMetaObject;
    const int interfaceInfoIndex = metaObject.indexOfClassInfo("D-Bus Interface");
    if (interfaceInfoIndex < 0) {
        qWarning() << metaObject.className() << "does not have 'D-Bus Interface' class info";
        return;
    }
    const QByteArrayView interface = metaObject.classInfo(interfaceInfoIndex).value();
    if (interface.isEmpty()) {
        qWarning() << "Didn't find dbus interface for class" << metaObject.className();
        return;
    }
    auto message = QDBusMessage::createTargetedSignal(u"org.freedesktop.portal.Desktop"_s,
                                                      u"/org/freedesktop/portal/desktop"_s,
                                                      QString::fromUtf8(interface),
                                                      QString::fromUtf8(metaSignal.name()));
    if constexpr (sizeof...(Args) > 0) {
        auto addArguments = [&message, &args...]<size_t... I>(std::index_sequence<I...>) {
            auto makeVariant = []<size_t index, typename T>(T &&value) {
                using targetType = std::remove_cvref_t<typename std::tuple_element_t<index, typename member_function_traits<Signal>::Arguments>>;
                return QVariant::fromValue<targetType>(std::forward<T>(value));
            };
            (message << ... << makeVariant.template operator()<I>(std::forward<Args>(args)));
        };
        addArguments(std::make_index_sequence<sizeof...(Args)>());
    }
    QDBusConnection::sessionBus().send(message);
}
