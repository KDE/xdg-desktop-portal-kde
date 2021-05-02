/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_ACCESS_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_ACCESS_DIALOG_H

#include <QDialog>

namespace Ui
{
class AccessDialog;
}

class AccessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AccessDialog(QDialog *parent = nullptr, Qt::WindowFlags flags = {});
    ~AccessDialog();

    void setAcceptLabel(const QString &label);
    void setBody(const QString &body);
    void setIcon(const QString &icon);
    void setRejectLabel(const QString &label);
    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);

private:
    Ui::AccessDialog *m_dialog;
};

#endif // XDG_DESKTOP_PORTAL_KDE_ACCESS_DIALOG_H
