/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "outputsmodel.h"
#include <KLocalizedString>
#include <QIcon>

OutputsModel::OutputsModel(Options o, QObject *parent)
    : QAbstractListModel(parent)
{
    const auto outputs = WaylandIntegration::screens().values();

    // Only show the full workspace if there's several outputs
    if (outputs.count() > 1 && (o & WorkspaceIncluded)) {
        m_outputs << Output{WaylandIntegration::WaylandOutput::Workspace, 0, i18n("Full Workspace"), "Workspace", {}};
    }

    if (o & VirtualIncluded) {
        static quint64 i = 0;
        m_outputs << Output{WaylandIntegration::WaylandOutput::Virtual, 0, i18n("New Virtual Output"), QStringLiteral("Virtual%1").arg(i++), {}};
    }

    if (o & RegionIncluded) {
        m_outputs << Output{WaylandIntegration::WaylandOutput::Region, 0, i18n("Rectangular Region"), "Region", {}};
    }

    for (auto output : outputs) {
        QString display;
        switch (output.outputType()) {
        case WaylandIntegration::WaylandOutput::Laptop:
            display = i18n("Laptop screen");
            break;
        default:
            display = output.model();
            break;
        }
        const QPoint pos = output.globalPosition();
        m_outputs
            << Output(output.outputType(), output.waylandOutputName(), display, QStringLiteral("%1x%2").arg(pos.x()).arg(pos.y()), output.output()->name());
    }
}

OutputsModel::~OutputsModel() = default;

int OutputsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_outputs.count();
}

QHash<int, QByteArray> OutputsModel::roleNames() const
{
    return QHash<int, QByteArray>{
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {Qt::CheckStateRole, "checked"},
        {OutputNameRole, "outputName"},
        {NameRole, "name"},
    };
}

QVariant OutputsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto &output = m_outputs[index.row()];
    switch (role) {
    case OutputNameRole:
        return output.waylandOutputName();
    case NameRole:
        return output.name();
    case Qt::DecorationRole:
        return QIcon::fromTheme(output.iconName());
    case Qt::DisplayRole:
        return output.display();
    case Qt::CheckStateRole:
        return m_selectedRows.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
    }
    return {};
}

bool OutputsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
        return false;
    }

    if (index.data(Qt::CheckStateRole) == value) {
        return true;
    }

    if (value == Qt::Checked) {
        m_selectedRows.insert(index.row());
    } else {
        m_selectedRows.remove(index.row());
    }
    Q_EMIT dataChanged(index, index, {role});
    if (m_selectedRows.count() <= 1) {
        Q_EMIT hasSelectionChanged();
    }
    return true;
}

const Output &OutputsModel::outputAt(int row) const
{
    return m_outputs[row];
}

void OutputsModel::clearSelection()
{
    if (m_selectedRows.isEmpty())
        return;

    auto selected = m_selectedRows;
    m_selectedRows.clear();
    for (int i = 0, c = rowCount({}); i < c; ++i) {
        if (selected.contains(i)) {
            const auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {Qt::CheckStateRole});
        }
    }
    Q_EMIT hasSelectionChanged();
}

QList<Output> OutputsModel::selectedOutputs() const
{
    QList<Output> ret;
    ret.reserve(m_selectedRows.count());
    for (auto x : qAsConst(m_selectedRows)) {
        ret << m_outputs[x];
    }
    return ret;
}
