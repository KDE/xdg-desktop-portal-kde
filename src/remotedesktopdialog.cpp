/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "remotedesktopdialog.h"
#include "remotedesktopdialog_debug.h"
#include "utils.h"

#include <KLocalizedString>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QWindow>

RemoteDesktopDialog::RemoteDesktopDialog(const QString &appName,
                                         RemoteDesktopPortal::DeviceTypes deviceTypes,
                                         bool screenSharingEnabled,
                                         bool multiple,
                                         QObject *parent)
    : QuickDialog(parent)
{
    // We disable sharing the full workspace, can be addressed eventually
    auto model = new OutputsModel(OutputsModel::None, this);

    QVariantMap props = {
        {"outputsModel", QVariant::fromValue<QObject *>(model)},
        {"withScreenSharing", screenSharingEnabled},
        {"withMultipleScreenSharing", multiple},
        {"withKeyboard", deviceTypes.testFlag(RemoteDesktopPortal::Keyboard)},
        {"withPointer", deviceTypes.testFlag(RemoteDesktopPortal::Pointer)},
        {"withTouch", deviceTypes.testFlag(RemoteDesktopPortal::TouchScreen)},
    };

    const QString applicationName = Utils::applicationName(appName);
    if (applicationName.isEmpty()) {
        props.insert(QStringLiteral("title"), i18n("Select what to share with the requesting application"));
    } else {
        props.insert(QStringLiteral("title"), i18n("Select what to share with %1", applicationName));
    }

    create(QStringLiteral("qrc:/RemoteDesktopDialog.qml"), props);
}

QList<Output> RemoteDesktopDialog::selectedOutputs() const
{
    OutputsModel *model = dynamic_cast<OutputsModel *>(m_theDialog->property("outputsModel").value<QObject *>());
    if (!model) {
        return {};
    }
    return model->selectedOutputs();
}

RemoteDesktopPortal::DeviceTypes RemoteDesktopDialog::deviceTypes() const
{
    RemoteDesktopPortal::DeviceTypes types = RemoteDesktopPortal::None;
    if (m_theDialog->property("withKeyboard").toBool())
        types |= RemoteDesktopPortal::Keyboard;
    if (m_theDialog->property("withPointer").toBool())
        types |= RemoteDesktopPortal::Pointer;
    if (m_theDialog->property("withTouch").toBool())
        types |= RemoteDesktopPortal::TouchScreen;
    return types;
}
