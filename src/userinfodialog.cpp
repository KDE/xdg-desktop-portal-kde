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

    QVariantMap props = {
        {u"mainText"_s, i18nc("@title", "Share user info with %1", appName)},
        {u"subtitle"_s, reason.isEmpty() ? i18nc("@info:usagetip", "Allows your username, full name, and profile picture to be used by the application.", appName) : reason},
        {u"username"_s, id()},
        {u"realname"_s, name()}};

    if (QFileInfo::exists(m_userInterface->iconFile())) {
        props.insert(QStringLiteral("avatar"), image());
    }
    create(u"UserInfoDialog"_s, props);
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
