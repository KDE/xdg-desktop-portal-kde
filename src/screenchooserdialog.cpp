/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "screenchooserdialog.h"
#include "ui_screenchooserdialog.h"
#include "waylandintegration.h"

#include <KLocalizedString>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardPaths>

class FilteredWindowModel : public QSortFilterProxyModel
{
public:
    FilteredWindowModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if (source_parent.isValid())
            return false;

        const auto idx = sourceModel()->index(source_row, 0);
        using KWayland::Client::PlasmaWindowModel;

        return !idx.data(PlasmaWindowModel::SkipTaskbar).toBool() //
            && !idx.data(PlasmaWindowModel::SkipSwitcher).toBool() //
            && idx.data(PlasmaWindowModel::Pid) != QCoreApplication::applicationPid();
    }
};

ScreenChooserDialog::ScreenChooserDialog(const QString &appName, bool multiple, QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_multiple(multiple)
    , m_dialog(new Ui::ScreenChooserDialog)
{
    m_dialog->setupUi(this);

    const auto selection = multiple ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection;
    m_dialog->screenView->setSelectionMode(selection);
    m_dialog->windowsView->setSelectionMode(selection);

    auto model = new KWayland::Client::PlasmaWindowModel(WaylandIntegration::plasmaWindowManagement());
    auto proxy = new FilteredWindowModel(this);
    proxy->setSourceModel(model);
    m_dialog->windowsView->setModel(proxy);

    connect(m_dialog->buttonBox, &QDialogButtonBox::accepted, this, &ScreenChooserDialog::accept);
    connect(m_dialog->buttonBox, &QDialogButtonBox::rejected, this, &ScreenChooserDialog::reject);
    connect(m_dialog->screenView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ScreenChooserDialog::selectionChanged);
    connect(m_dialog->windowsView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ScreenChooserDialog::selectionChanged);
    connect(m_dialog->screenView, &QListWidget::doubleClicked, this, &ScreenChooserDialog::accept);
    connect(m_dialog->windowsView, &QListView::doubleClicked, this, &ScreenChooserDialog::accept);

    auto okButton = m_dialog->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText(i18n("Share"));
    okButton->setEnabled(false);

    QString applicationName;
    const QString desktopFile = appName + QLatin1String(".desktop");
    const QStringList desktopFileLocations = QStandardPaths::locateAll(QStandardPaths::ApplicationsLocation, desktopFile, QStandardPaths::LocateFile);
    for (const QString &location : desktopFileLocations) {
        QSettings settings(location, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("Desktop Entry"));
        if (settings.contains(QStringLiteral("X-GNOME-FullName"))) {
            applicationName = settings.value(QStringLiteral("X-GNOME-FullName")).toString();
        } else {
            applicationName = settings.value(QStringLiteral("Name")).toString();
        }

        if (!applicationName.isEmpty()) {
            break;
        }
    }

    if (applicationName.isEmpty()) {
        setWindowTitle(i18n("Select screen to share with the requesting application"));
    } else {
        setWindowTitle(i18n("Select screen to share with %1", applicationName));
    }
}

ScreenChooserDialog::~ScreenChooserDialog()
{
    delete m_dialog;
}

void ScreenChooserDialog::selectionChanged(const QItemSelection &selected)
{
    if (!m_multiple && !selected.isEmpty()) {
        if (selected.constFirst().model() == m_dialog->windowsView->model())
            m_dialog->screenView->clearSelection();
        else
            m_dialog->windowsView->clearSelection();
    }

    auto okButton = m_dialog->buttonBox->button(QDialogButtonBox::Ok);
    const auto count = m_dialog->screenView->selectionModel()->hasSelection() + m_dialog->windowsView->selectionModel()->hasSelection();
    okButton->setEnabled(m_multiple ? count > 0 : count == 1);
}

void ScreenChooserDialog::setSourceTypes(ScreenCastPortal::SourceTypes types)
{
    m_dialog->windowsTab->setEnabled(types & ScreenCastPortal::Window);
    m_dialog->screensTab->setEnabled(types & ScreenCastPortal::Monitor);
    if (!m_dialog->screensTab->isEnabled()) {
        m_dialog->tabWidget->setCurrentWidget(m_dialog->windowsTab);
    }
}

QList<quint32> ScreenChooserDialog::selectedScreens() const
{
    return m_dialog->screenView->selectedScreens();
}

QList<QByteArray> ScreenChooserDialog::selectedWindows() const
{
    const auto idxs = m_dialog->windowsView->selectionModel()->selectedIndexes();

    QList<QByteArray> ret;
    ret.reserve(idxs.count());
    for (const auto &idx : idxs) {
        ret += idx.data(KWayland::Client::PlasmaWindowModel::Uuid).toByteArray();
    }
    return ret;
}
