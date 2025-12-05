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

struct UserDetails {
private:
    // This must never be public, all properties should be cached!
    OrgFreedesktopAccountsUserInterface m_userInterface{QStringLiteral("org.freedesktop.Accounts"),
                                                        QStringLiteral("/org/freedesktop/Accounts/User%1").arg(getuid()),
                                                        QDBusConnection::systemBus()};

public:
    // Cache properties so we don't need to look them up multiple times for the same dialog
    QString m_iconFile = m_userInterface.iconFile();
    bool m_iconFileExists = QFileInfo::exists(m_iconFile);
    QString m_realName = m_userInterface.realName();
    QString m_userName = m_userInterface.userName();
};

UserInfoDialog::UserInfoDialog(const QString &reason, const QString &app_id, QObject *parent)
    : QuickDialog(parent)
    , m_userDetails(std::make_unique<UserDetails>())
{
    const KService::Ptr app = KService::serviceByDesktopName(app_id);
    const QString appName = app ? app->name() : app_id;

    QVariantMap props = {
        {u"mainText"_s, i18nc("@title", "Share user info with %1", appName)},
        {u"subtitle"_s,
         reason.isEmpty() ? i18nc("@info:usagetip", "Allows your username, full name, and profile picture to be used by the application.", appName) : reason},
        {u"username"_s, id()},
        {u"realname"_s, name()},
        {u"avatar"_s, image()},
    };

    create(u"UserInfoDialog"_s, props);
}

UserInfoDialog::~UserInfoDialog() = default;

QString UserInfoDialog::id() const
{
    return m_userDetails->m_userName;
}

QString UserInfoDialog::image() const
{
    // TODO: Ideally plasma-welcome and kcm_users should always set an avatar (if need be to a render of Kirigami.Avatar)
    // such that the user always has a valid avatar 99.9% of the time.
    if (!m_userDetails->m_iconFileExists) {
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
    return QUrl::fromLocalFile(m_userDetails->m_iconFile).toString();
}

QString UserInfoDialog::name() const
{
    return m_userDetails->m_realName;
}

#include "moc_userinfodialog.cpp"
