/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H

#include "quickdialog.h"

class OrgFreedesktopAccountsUserInterface;
struct UserDetails;

class UserInfoDialog : public QuickDialog
{
    Q_OBJECT
public:
    explicit UserInfoDialog(const QString &reason, const QString &app_id, QObject *parent = nullptr);
    ~UserInfoDialog() override;
    Q_DISABLE_COPY_MOVE(UserInfoDialog)

    QString id() const;
    QString name() const;
    QString image() const;

private:
    std::unique_ptr<UserDetails> m_userDetails;
};

#endif // XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H
