/*
 * Copyright © 2017 Red Hat, Inc
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
#include "appchooserdialogitem.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <KLocalizedString>
#include <QSettings>
#include <QStandardPaths>

#include <KProcess>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeAppChooserDialog, "xdp-kde-app-chooser-dialog")

AppChooserDialog::AppChooserDialog(const QStringList &choices, const QString &defaultApp, const QString &fileName, QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
    setMinimumWidth(640);

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->setSpacing(20);
    vboxLayout->setMargin(20);

    QLabel *label = new QLabel(this);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->setScaledContents(true);
    label->setWordWrap(true);
    label->setText(i18n("Select application to open \"%1\". Other applications are available in <a href=#discover><span style=\"text-decoration: underline\">Discover</span></a>.", fileName));
    label->setOpenExternalLinks(false);

    connect(label, &QLabel::linkActivated, this, [] () {
        KProcess::startDetached("plasma-discover");
    });

    vboxLayout->addWidget(label);

    QGridLayout *gridLayout = new QGridLayout();

    int i = 0, j = 0;
    foreach (const QString &choice, choices) {
        const QString desktopFile = choice + QLatin1String(".desktop");
        const QStringList desktopFilesLocations = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, desktopFile, QStandardPaths::LocateFile);
        foreach (const QString desktopFile, desktopFilesLocations) {
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

            AppChooserDialogItem *item = new AppChooserDialogItem(applicationName, applicationIcon, choice);
            gridLayout->addWidget(item, i, j++, Qt::AlignHCenter);

            connect(item, &AppChooserDialogItem::doubleClicked, this, [this] (const QString &selectedApplication) {
                m_selectedApplication = selectedApplication;
                QDialog::accept();
            });

            if (choice == defaultApp) {
                item->setDown(true);
                item->setChecked(true);
            }

            if (j == 3) {
                i++;
                j = 0;
            }
        }
    }

    vboxLayout->addLayout(gridLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    vboxLayout->addWidget(buttonBox, 0, Qt::AlignBottom | Qt::AlignRight);

    setLayout(vboxLayout);
    setWindowTitle(i18n("Open with"));
}

AppChooserDialog::~AppChooserDialog()
{
}

QString AppChooserDialog::selectedApplication() const
{
    return m_selectedApplication;
}
