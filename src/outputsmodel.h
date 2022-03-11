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
    Output(WaylandIntegration::WaylandOutput::OutputType outputType, int waylandOutputName, const QString &display)
        : m_outputType(outputType)
        , m_waylandOutputName(waylandOutputName)
        , m_display(display)
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

    WaylandIntegration::WaylandOutput::OutputType outputType() const
    {
        return m_outputType;
    }

private:
    WaylandIntegration::WaylandOutput::OutputType m_outputType;
    int m_waylandOutputName;
    QString m_display;
};

class OutputsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
public:
    enum Options {
        None = 0,
        WorkspaceIncluded,
    };
    Q_ENUM(Options);

    OutputsModel(Options o, QObject *parent);
    ~OutputsModel() override;

    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

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
