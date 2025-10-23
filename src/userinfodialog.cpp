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

#include <KIconLoader>
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
        {u"title"_s, i18n("Share User Information with %1", appName)},
        {u"subtitle"_s, reason.isEmpty() ? i18n("%1 wants access to your user info.", appName) : reason},
        {u"iconName"_s, QStringLiteral("user-identity")},
        {u"details"_s, i18n("This app will be given access to your avatar and name.")},
        {u"name"_s, m_userInterface->realName().isEmpty() ? m_userInterface->userName() : m_userInterface->realName()},
        {u"avatar"_s, image()},
    };

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
    // TODO: Ideally plasma-welcome and kcm_users should always set an avatar (if need be to a render of Kirigami.Avatar)
    // such that the user always has a valid avatar 99.9% of the time.
    if (!QFileInfo::exists(m_userInterface->iconFile())) {
        // Always provide a fallback. We **must** provide an avatar to XDP per its documentation and code.
        // Legacy users may not have one, systems without Accounts service may not have one, some software may clear it...
        static const auto iconURI = [] {
            const auto iconPath = KIconLoader::global()->iconPath(u"user"_s, -KIconLoader::SizeEnormous, /* canReturnNull = */ false);
            return u"file://"_s + QFileInfo(iconPath).canonicalFilePath();
        }();
        // WARNING: do not use symlinks. They make XDP forward incorrect paths to clients because it will portal the canonical path
        //   but forward the original (symlink) path
        return iconURI;
    }
    return QUrl::fromLocalFile(m_userInterface->iconFile()).toString();
}

QString UserInfoDialog::name() const
{
    return m_userInterface->realName();
}
