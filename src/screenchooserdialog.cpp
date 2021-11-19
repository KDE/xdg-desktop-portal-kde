/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screenchooserdialog.h"
#include "outputsmodel.h"
#include "utils.h"

#include <KLocalizedString>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>

#include <QCoreApplication>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QWindow>

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

    QMap<int, QVariant> itemData(const QModelIndex &index) const override
    {
        using KWayland::Client::PlasmaWindowModel;
        auto ret = QSortFilterProxyModel::itemData(index);
        for (int i = PlasmaWindowModel::AppId; i <= PlasmaWindowModel::Uuid; ++i) {
            ret[i] = index.data(i);
        }
        return ret;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
            return false;
        }

        using KWayland::Client::PlasmaWindowModel;
        const QString uuid = index.data(PlasmaWindowModel::Uuid).toString();
        if (value == Qt::Checked) {
            m_selected.insert(index);
        } else {
            m_selected.remove(index);
        }
        Q_EMIT dataChanged(index, index, {role});
        return true;
    }

    QVector<QMap<int, QVariant>> selectedWindows() const
    {
        QVector<QMap<int, QVariant>> ret;
        ret.reserve(m_selected.size());
        for (const auto &index : m_selected) {
            if (index.isValid())
                ret << itemData(index);
        }
        return ret;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> ret = sourceModel()->roleNames();
        ret.insert(Qt::CheckStateRole, "checked");
        return ret;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
            return {};
        }

        switch (role) {
        case Qt::CheckStateRole:
            return m_selected.contains(index) ? Qt::Checked : Qt::Unchecked;
        default:
            return QSortFilterProxyModel::data(index, role);
        }
        return {};
    }
    void clearSelection()
    {
        auto selected = m_selected;
        m_selected.clear();

        for (const auto &index : selected) {
            if (index.isValid())
                Q_EMIT dataChanged(index, index, {Qt::CheckStateRole});
        }
    }

private:
    QSet<QPersistentModelIndex> m_selected;
};

ScreenChooserDialog::ScreenChooserDialog(const QString &appName, bool multiple, ScreenCastPortal::SourceTypes types)
    : QuickDialog()
{
    const QString applicationName = Utils::applicationName(appName);
    QVariantMap props = {
        {"title", i18n("Screen Sharing")},
        {"mainText",
         appName.isEmpty() ? i18n("Select screen to share with the requesting application") : i18n("Select what to share with %1", applicationName)},
        {"multiple", multiple},
    };
    Q_ASSERT(types != 0);
    if (types & ScreenCastPortal::Monitor) {
        auto model = new OutputsModel(this);
        props.insert("outputsModel", QVariant::fromValue<QObject *>(model));
        connect(this, &ScreenChooserDialog::clearSelection, model, &OutputsModel::clearSelection);
    }
    if (types & ScreenCastPortal::Window) {
        auto model = new KWayland::Client::PlasmaWindowModel(WaylandIntegration::plasmaWindowManagement());
        auto windowsProxy = new FilteredWindowModel(this);
        windowsProxy->setSourceModel(model);
        props.insert("windowsModel", QVariant::fromValue<QObject *>(windowsProxy));
        connect(this, &ScreenChooserDialog::clearSelection, windowsProxy, &FilteredWindowModel::clearSelection);
    }

    create("qrc:/ScreenChooserDialog.qml", props);
    connect(m_theDialog, SIGNAL(clearSelection()), this, SIGNAL(clearSelection()));
}

ScreenChooserDialog::~ScreenChooserDialog() = default;

QList<quint32> ScreenChooserDialog::selectedScreens() const
{
    OutputsModel *model = dynamic_cast<OutputsModel *>(m_theDialog->property("outputsModel").value<QObject *>());
    if (!model) {
        return {};
    }
    return model->selectedScreens();
}

QVector<QMap<int, QVariant>> ScreenChooserDialog::selectedWindows() const
{
    FilteredWindowModel *model = dynamic_cast<FilteredWindowModel *>(m_theDialog->property("windowsModel").value<QObject *>());
    if (!model) {
        return {};
    }
    return model->selectedWindows();
}
