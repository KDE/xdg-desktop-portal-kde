/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#include "accessdialog.h"

#include "utils.h"
#include <KLocalizedString>
#include <QLoggingCategory>
#include <QPushButton>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAccessDialog, "xdp-kde-access-dialog")

AccessDialog::AccessDialog(QObject *parent)
    : QuickDialog(parent)
{
    m_props = {
        {"iconName", "dialog-question"},
        {"title", i18n("Request device access")},
    };
}

void AccessDialog::setAcceptLabel(const QString &label)
{
    m_props.insert("acceptLabel", label);
}

void AccessDialog::setBody(const QString &body)
{
    m_props.insert("body", body);
}

void AccessDialog::setIcon(const QString &icon)
{
    m_props.insert("iconName", icon);
}

void AccessDialog::setRejectLabel(const QString &label)
{
    m_props.insert("rejectLabel", label);
}

void AccessDialog::setSubtitle(const QString &subtitle)
{
    m_props.insert("subtitle", subtitle);
}

void AccessDialog::setTitle(const QString &title)
{
    m_props.insert("title", title);
}

void AccessDialog::createDialog()
{
    create("qrc:/AccessDialog.qml", m_props);
}
