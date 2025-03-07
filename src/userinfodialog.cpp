/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 */

#include "userinfodialog.h"

#include "user_interface.h"

#include <sys/types.h>
#include <unistd.h>

#include <QFileInfo>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KService>

using namespace Qt::StringLiterals;

UserInfoDialog::UserInfoDialog(const QString &reason, const QString &app_id, QObject *parent)
    : QuickDialog(parent)
{
    QString ifacePath = QStringLiteral("/org/freedesktop/Accounts/User%1").arg(getuid());
    m_userInterface = new OrgFreedesktopAccountsUserInterface(QStringLiteral("org.freedesktop.Accounts"), ifacePath, QDBusConnection::systemBus(), this);

    const KService::Ptr app = KService::serviceByDesktopName(app_id);
    const QString appName = app ? app->name() : app_id;

    QVariantMap props = {{u"title"_s, i18n("Share User Information with %1", appName)},
                         {u"subtitle"_s, reason.isEmpty() ? i18n("%1 wants access to your user info.", appName) : reason},
                         {u"iconName"_s, QStringLiteral("user-identity")},
                         {u"details"_s, i18n("This app will be given access to your avatar and name.")},
                         {u"name"_s, m_userInterface->realName().isEmpty() ? m_userInterface->userName() : m_userInterface->realName()}};

    if (QFileInfo::exists(m_userInterface->iconFile())) {
        props.insert(QStringLiteral("avatar"), image());
    }
    create(u"qrc:/UserInfoDialog.qml"_s, props);
}

UserInfoDialog::~UserInfoDialog()
{
}

QString UserInfoDialog::id() const
{
    return m_userInterface->userName();
}

QString UserInfoDialog::image() const
{
    if (!QFileInfo::exists(m_userInterface->iconFile())) {
        return {};
    }
    return QUrl::fromLocalFile(m_userInterface->iconFile()).toString();
}

QString UserInfoDialog::name() const
{
    return m_userInterface->realName();
}
