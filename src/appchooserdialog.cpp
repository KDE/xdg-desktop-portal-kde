/*
 * Copyright © 2016 Red Hat, Inc
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

#include "appchooserdialog.h"
#include "ui_appchooserdialog.h"

#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>
#include <QPushButton>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAppChooserDialog, "xdg-desktop-portal-kde-app-chooser-dialog")

AppChooserDialog::AppChooserDialog(const QStringList &choices, QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_dialog(new Ui::AppChooserDialog)
    , m_choices(choices)
{
    m_dialog->setupUi(this);

    Q_FOREACH (const QString &choice, m_choices) {
        const QString desktopFile = choice + QLatin1String(".desktop");
        const QStringList desktopFilesLocations = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, desktopFile, QStandardPaths::LocateFile);
        Q_FOREACH (const QString desktopFile, desktopFilesLocations) {
            QString applicationIcon;
            QString applicationName;
            QSettings settings(desktopFile, QSettings::IniFormat);
            settings.beginGroup(QLatin1String("Desktop Entry"));
            if (settings.contains(QLatin1String("X-GNOME-FullName"))) {
                applicationName = settings.value(QLatin1String("X-GNOME-FullName")).toString();
            } else {
                applicationName = settings.value(QLatin1String("Name")).toString();
            }
            applicationIcon = settings.value(QLatin1String("Icon")).toString();

            QListWidgetItem *widgetItem = new QListWidgetItem(m_dialog->appView);
            widgetItem->setData(Qt::UserRole, QVariant::fromValue(choice));
            // FIXME GTK icons will work only with Qt 5.7
            widgetItem->setIcon(QIcon::fromTheme(applicationIcon));
            widgetItem->setText(applicationName);
        }
    }

    connect(m_dialog->buttonBox, &QDialogButtonBox::accepted, this, &AppChooserDialog::accept);
    connect(m_dialog->buttonBox, &QDialogButtonBox::rejected, this, &AppChooserDialog::reject);
    connect(m_dialog->appView, &QListWidget::itemDoubleClicked, this, &AppChooserDialog::accept);
    connect(m_dialog->searchEdit, &QLineEdit::textChanged, this, &AppChooserDialog::searchTextChanged);

    m_dialog->buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("Select"));
    setWindowTitle(i18n("Select application"));
}

AppChooserDialog::~AppChooserDialog()
{
    delete m_dialog;
}

QString AppChooserDialog::selectedApplication() const
{
    return m_dialog->appView->currentItem()->data(Qt::UserRole).toString();
}

void AppChooserDialog::setSelectedApplication(const QString &applicationName)
{
    for (int i = 0; i < m_dialog->appView->count(); i++) {
        QListWidgetItem *widgetItem = m_dialog->appView->item(i);
        if (widgetItem->data(Qt::UserRole).toString() == applicationName) {
            m_dialog->appView->setCurrentItem(widgetItem, QItemSelectionModel::Select);
        }
    }
}

void AppChooserDialog::searchTextChanged(const QString &text)
{
    for (int i = 0; i < m_dialog->appView->count(); i++) {
        QListWidgetItem *widgetItem = m_dialog->appView->item(i);
        if (text.isEmpty()) {
            widgetItem->setHidden(false);
        } else {
            widgetItem->setHidden(!widgetItem->text().toLower().contains(text.toLower()));
        }
    }
}
