/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#include "usb.h"

#include "dbushelpers.h"
#include "quickdialog.h"
#include "request.h"
#include "utils.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QWindow>

using namespace Qt::StringLiterals;

#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
template<typename... T>
QDBusArgument &operator<<(QDBusArgument &argument, const std::tuple<T...> &tuple)
{
    static_assert(std::tuple_size_v<std::tuple<T...>> != 0, "D-Bus doesn't allow empty structs");
    argument.beginStructure();
    std::apply(
        [&argument](const auto &...elements) {
            (argument << ... << elements);
        },
        tuple);
    argument.endStructure();
    return argument;
}

template<typename... T>
const QDBusArgument &operator>>(const QDBusArgument &argument, std::tuple<T...> &tuple)
{
    static_assert(std::tuple_size_v<std::tuple<T...>> != 0, "D-Bus doesn't allow empty structs");
    argument.beginStructure();
    std::apply(
        [&argument](auto &...elements) {
            (argument >> ... >> elements);
        },
        tuple);
    argument.endStructure();
    return argument;
}
#endif

struct Device {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id CONSTANT)
    Q_PROPERTY(bool writable MEMBER writable CONSTANT)
    Q_PROPERTY(QVariantMap properties MEMBER properties CONSTANT)
public:
    QString id;
    bool writable;
    QVariantMap properties;
};

UsbPortal::UsbPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<std::tuple<QString, QVariantMap, QVariantMap>>();
    qDBusRegisterMetaType<QList<std::tuple<QString, QVariantMap, QVariantMap>>>();
}

void UsbPortal::AcquireDevices(const QDBusObjectPath &handle,
                               const QString &parent_window,
                               const QString &app_id,
                               const QList<std::tuple<QString, QVariantMap, QVariantMap>> &devices,
                               [[maybe_unused]] const QVariantMap &options,
                               const QDBusMessage &message,
                               [[maybe_unused]] uint &replyResponse,
                               [[maybe_unused]] QVariantMap &replyResults)
{
    auto buildDevice = [](const std::tuple<QString, QVariantMap, QVariantMap> &device) {
        return QVariant::fromValue(Device{
            .id = std::get<0>(device),
            .writable = std::get<2>(device).value(u"writable"_s, false).toBool(),
            .properties = qdbus_cast<QVariantMap>(std::get<1>(device).value(u"properties"_s)),
        });
    };
    QList<QVariant> deviceList;
    deviceList.reserve(devices.size());
    std::ranges::transform(devices, std::back_inserter(deviceList), buildDevice);

    auto dialog = new QuickDialog;
    Utils::setParentWindow(dialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequest(handle, dialog);
    dialog->create(u"UsbDialog"_s, {{u"app"_s, Utils::applicationName(app_id)}, {u"devices"_s, QVariant::fromValue(deviceList)}});

    delayReply(message, dialog, this, [dialog](DialogResult result) -> QVariantList {
        if (result == DialogResult::Rejected) {
            return {PortalResponse::Cancelled, QVariantMap{}};
        }
        QList<std::pair<QString, QVariantMap>> acceptedDevices;
        auto devices = dialog->windowHandle()->property("devices").toList();
        for (const auto &deviceVariant : devices) {
            const auto device = deviceVariant.value<Device>();
            acceptedDevices.emplace_back(device.id, QVariantMap{{u"writable"_s, device.writable}});
        }
        return {PortalResponse::Success, QVariantMap{{u"devices"_s, QVariant::fromValue(acceptedDevices)}}};
    });
}

#include "usb.moc"
