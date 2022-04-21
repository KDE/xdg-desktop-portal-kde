/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "waylandintegration.h"
#include <QAbstractListModel>
#include <QSet>

class Output
{
public:
    Output(WaylandIntegration::WaylandOutput::OutputType outputType, int waylandOutputName, const QString &display, const QString &uniqueId)
        : m_outputType(outputType)
        , m_waylandOutputName(waylandOutputName)
        , m_display(display)
        , m_uniqueId(uniqueId)
    {
    }

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

    QString uniqueId() const
    {
        return m_uniqueId;
    }

    WaylandIntegration::WaylandOutput::OutputType outputType() const
    {
        return m_outputType;
    }

private:
    WaylandIntegration::WaylandOutput::OutputType m_outputType;
    int m_waylandOutputName;
    QString m_display;
    QString m_uniqueId;
};

class OutputsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
public:
    enum Option {
        None = 0,
        WorkspaceIncluded = 0x1,
        VirtualIncluded = 0x2,
    };
    Q_ENUM(Option);
    Q_DECLARE_FLAGS(Options, Option);

    OutputsModel(Options o, QObject *parent);
    ~OutputsModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    const Output &outputAt(int row) const;
    QList<Output> selectedOutputs() const;
    bool hasSelection() const
    {
        return !m_selectedRows.isEmpty();
    }

public Q_SLOTS:
    void clearSelection();

Q_SIGNALS:
    void hasSelectionChanged();

private:
    QList<Output> m_outputs;
    QSet<quint32> m_selectedRows;
};
