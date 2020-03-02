/*
 * Copyright © 2020 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
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

