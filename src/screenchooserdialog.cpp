/*
 * Copyright © 2018 Red Hat, Inc
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

#include "screenchooserdialog.h"
#include "ui_screenchooserdialog.h"

#include <KLocalizedString>
#include <QPushButton>

ScreenChooserDialog::ScreenChooserDialog(bool multiple, QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_dialog(new Ui::ScreenChooserDialog)
{
    m_dialog->setupUi(this);

    if (multiple) {
        m_dialog->screenView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    connect(m_dialog->buttonBox, &QDialogButtonBox::accepted, this, &ScreenChooserDialog::accept);
    connect(m_dialog->buttonBox, &QDialogButtonBox::rejected, this, &ScreenChooserDialog::reject);
    connect(m_dialog->screenView, &QListWidget::itemDoubleClicked, this, &ScreenChooserDialog::accept);

    m_dialog->buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("Share"));
    setWindowTitle(i18n("Select screen to share"));
}

ScreenChooserDialog::~ScreenChooserDialog()
{
    delete m_dialog;
}

QList<quint32> ScreenChooserDialog::selectedScreens() const
{
    return m_dialog->screenView->selectedScreens();
}
