/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "accessdialog.h"
#include "ui_accessdialog.h"

#include <QLoggingCategory>
#include <QPushButton>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAccessDialog, "xdp-kde-access-dialog")

AccessDialog::AccessDialog(QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_dialog(new Ui::AccessDialog)
{
    m_dialog->setupUi(this);

    connect(m_dialog->buttonBox, &QDialogButtonBox::accepted, this, &AccessDialog::accept);
    connect(m_dialog->buttonBox, &QDialogButtonBox::rejected, this, &AccessDialog::reject);

    m_dialog->iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-question")).pixmap(QSize(64, 64)));

    setWindowTitle(i18n("Request device access"));
}

AccessDialog::~AccessDialog()
{
    delete m_dialog;
}

void AccessDialog::setAcceptLabel(const QString &label)
{
    m_dialog->buttonBox->button(QDialogButtonBox::Ok)->setText(label);
}

void AccessDialog::setBody(const QString &body)
{
    m_dialog->bodyLabel->setText(body);
}

void AccessDialog::setIcon(const QString &icon)
{
    m_dialog->iconLabel->setPixmap(QIcon::fromTheme(icon).pixmap(QSize(64, 64)));
}

void AccessDialog::setRejectLabel(const QString &label)
{
    m_dialog->buttonBox->button(QDialogButtonBox::Cancel)->setText(label);
}

void AccessDialog::setSubtitle(const QString &subtitle)
{
    m_dialog->subtitleLabel->setText(subtitle);
}

void AccessDialog::setTitle(const QString &title)
{
    m_dialog->titleLabel->setText(title);
}
