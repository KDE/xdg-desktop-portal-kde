/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "waylandintegration.h"
#include <QAbstractListModel>
#include <QSet>

class OutputsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
public:
    OutputsModel(QObject *parent);

    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    QList<quint32> selectedScreens() const;
    bool hasSelection() const
    {
        return !m_selected.isEmpty();
    }

public Q_SLOTS:
    void clearSelection();

Q_SIGNALS:
    void hasSelectionChanged();

private:
    QList<WaylandIntegration::WaylandOutput> m_outputs;
    QSet<quint32> m_selected;
};
