/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H

#include <QDialog>

namespace Ui
{
class UserInfoDialog;
}

class OrgFreedesktopAccountsUserInterface;

class UserInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UserInfoDialog(const QString &reason, QDialog *parent = nullptr, Qt::WindowFlags flags = {});
    ~UserInfoDialog();

    QString id() const;
    QString name() const;
    QString image() const;

private:
    Ui::UserInfoDialog *m_dialog;
    OrgFreedesktopAccountsUserInterface *m_userInterface;
};

#endif // XDG_DESKTOP_PORTAL_KDE_USERINFO_DIALOG_H
