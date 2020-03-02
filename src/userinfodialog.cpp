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

#include "userinfodialog.h"
#include "ui_userinfodialog.h"

#include "user_interface.h"

#include <unistd.h>
#include <sys/types.h>

#include <QFileInfo>
#include <QStandardPaths>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QQuickItem>

#include <kdeclarative/kdeclarative.h>

UserInfoDialog::UserInfoDialog(const QString &reason, QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_dialog(new Ui::UserInfoDialog)
{
    m_dialog->setupUi(this);

    KDeclarative::KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_dialog->quickWidget->engine());
    kdeclarative.setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    kdeclarative.setupEngine(m_dialog->quickWidget->engine());
    kdeclarative.setupContext();

    QString ifacePath = QStringLiteral("/org/freedesktop/Accounts/User%1").arg(getuid());
    m_userInterface = new OrgFreedesktopAccountsUserInterface(QStringLiteral("org.freedesktop.Accounts"), ifacePath, QDBusConnection::systemBus(), this);

    QString image = QFileInfo::exists(m_userInterface->iconFile()) ? m_userInterface->iconFile() : QString();

    m_dialog->quickWidget->rootContext()->setContextProperty(QStringLiteral("reason"), reason);
    m_dialog->quickWidget->rootContext()->setContextProperty(QStringLiteral("id"), m_userInterface->userName());
    m_dialog->quickWidget->rootContext()->setContextProperty(QStringLiteral("name"), m_userInterface->realName());
    if (QFileInfo::exists(m_userInterface->iconFile())) {
        m_dialog->quickWidget->rootContext()->setContextProperty(QStringLiteral("image"), m_userInterface->iconFile());
    } else {
        m_dialog->quickWidget->rootContext()->setContextProperty(QStringLiteral("image"), QIcon::fromTheme(QStringLiteral("user-identity")));
    }
    m_dialog->quickWidget->setClearColor(Qt::transparent);
    m_dialog->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_dialog->quickWidget->setSource(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("xdg-desktop-portal-kde/qml/UserInfoDialog.qml"))));

    QObject *rootItem = m_dialog->quickWidget->rootObject();
    connect(rootItem, SIGNAL(accepted()), this, SLOT(accept()));
    connect(rootItem, SIGNAL(rejected()), this, SLOT(reject()));

    setWindowTitle(i18n("Share Information"));
}

UserInfoDialog::~UserInfoDialog()
{
    delete m_dialog;
}

QString UserInfoDialog::id() const
{
    return m_userInterface->userName();
}

QString UserInfoDialog::image() const
{
    return  m_userInterface->realName();
}

QString UserInfoDialog::name() const
{
    return m_userInterface->iconFile();
}
