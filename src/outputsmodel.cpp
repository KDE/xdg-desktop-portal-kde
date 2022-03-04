/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "outputsmodel.h"
#include <KLocalizedString>
#include <QIcon>

class Output
{
public:
    int waylandOutputName() const
    {
        return m_waylandOutputName;
    }

    QString iconName() const
    {
        switch (m_outputType) {
        case WaylandIntegration::WaylandOutput::Laptop:
            return QStringLiteral("computer-laptop");
        case WaylandIntegration::WaylandOutput::Television:
            return QStringLiteral("video-television");
        default:
        case WaylandIntegration::WaylandOutput::Monitor:
            return QStringLiteral("video-display");
        }
    }

    QString display() const
    {
        return m_display;
    }

    WaylandIntegration::WaylandOutput::OutputType m_outputType;
    int m_waylandOutputName;
    QString m_display;
};

OutputsModel::OutputsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const auto outputs = WaylandIntegration::screens().values();

    // Only show the full workspace if there's several outputs
    if (outputs.count() > 1) {
        m_outputs << Output{WaylandIntegration::WaylandOutput::Monitor, 0, i18n("Full Workspace")};
    }
    for (auto output : outputs) {
        QString display;
        switch (output.outputType()) {
        case WaylandIntegration::WaylandOutput::Laptop:
            display = i18n("Laptop screen\nModel: %1", output.model());
            break;
        default:
            display = i18n("Manufacturer: %1\nModel: %2", output.manufacturer(), output.model());
            break;
        }
        m_outputs << Output{output.outputType(), output.waylandOutputName(), display};
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
        {Qt::UserRole, "outputName"},
        {Qt::CheckStateRole, "checked"},
    };
}

QVariant OutputsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto &output = m_outputs[index.row()];
    switch (role) {
    case Qt::UserRole:
        return output.waylandOutputName();
    case Qt::DecorationRole:
        return QIcon::fromTheme(output.iconName());
    case Qt::DisplayRole:
        return output.display();
    case Qt::CheckStateRole:
        return m_selected.contains(output.waylandOutputName()) ? Qt::Checked : Qt::Unchecked;
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

    const auto &output = m_outputs[index.row()];
    if (value == Qt::Checked) {
        m_selected.insert(output.waylandOutputName());
    } else {
        m_selected.remove(output.waylandOutputName());
    }
    Q_EMIT dataChanged(index, index, {role});
    if (m_selected.count() <= 1) {
        Q_EMIT hasSelectionChanged();
    }
    return true;
}

QList<quint32> OutputsModel::selectedScreens() const
{
    return m_selected.values();
}

void OutputsModel::clearSelection()
{
    if (m_selected.isEmpty())
        return;

    auto selected = m_selected;
    m_selected.clear();
    int i = 0;
    for (const auto &output : qAsConst(m_outputs)) {
        if (selected.contains(output.waylandOutputName())) {
            const auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {Qt::CheckStateRole});
        }
        ++i;
    }
    Q_EMIT hasSelectionChanged();
}
