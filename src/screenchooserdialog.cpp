/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screenchooserdialog.h"
#include "ui_screenchooserdialog.h"
#include "waylandintegration.h"

#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
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

class OutputsModel : public QAbstractListModel
{
public:
    OutputsModel(QObject *parent)
        : QAbstractListModel(parent)
    {
        m_outputs = WaylandIntegration::screens().values();
    }

    int rowCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : m_outputs.count();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return QHash<int, QByteArray>{
            {Qt::DisplayRole, "display"},
            {Qt::DecorationRole, "decoration"},
            {Qt::UserRole, "outputName"},
            {Qt::CheckStateRole, "checked"},
        };
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
            return {};
        }

        const auto &output = m_outputs[index.row()];
        switch (role) {
        case Qt::UserRole:
            return output.waylandOutputName();
        case Qt::DecorationRole:
            switch (output.outputType()) {
            case WaylandIntegration::WaylandOutput::Laptop:
                return QIcon::fromTheme(QStringLiteral("computer-laptop"));
            case WaylandIntegration::WaylandOutput::Monitor:
                return QIcon::fromTheme(QStringLiteral("video-display"));
            case WaylandIntegration::WaylandOutput::Television:
                return QIcon::fromTheme(QStringLiteral("video-television"));
            }
        case Qt::DisplayRole:
            switch (output.outputType()) {
            case WaylandIntegration::WaylandOutput::Laptop:
                return i18n("Laptop screen\nModel: %1", output.model());
            default:
                return i18n("Manufacturer: %1\nModel: %2", output.manufacturer(), output.model());
            }
        case Qt::CheckStateRole:
            return m_selected.contains(output.waylandOutputName()) ? Qt::Checked : Qt::Unchecked;
        }
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
            return false;
        }

        const auto &output = m_outputs[index.row()];
        if (value == Qt::Checked) {
            m_selected.insert(output.waylandOutputName());
        } else {
            m_selected.remove(output.waylandOutputName());
        }
        Q_EMIT dataChanged(index, index, {role});
        return true;
    }

    QList<quint32> selectedScreens() const
    {
        return m_selected.values();
    }
    void clearSelection()
    {
        auto selected = m_selected;
        m_selected.clear();
        int i = 0;
        for (const auto &output : m_outputs) {
            if (selected.contains(output.waylandOutputName())) {
                const auto idx = index(i, 0);
                Q_EMIT dataChanged(idx, idx, {Qt::CheckStateRole});
            }
            ++i;
        }
    }

private:
    QList<WaylandIntegration::WaylandOutput> m_outputs;
    QSet<quint32> m_selected;
};

ScreenChooserDialog::ScreenChooserDialog(const QString &appName, bool multiple, ScreenCastPortal::SourceTypes types)
    : QuickDialog()
{
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
