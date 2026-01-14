/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QScreen>
#include <QSet>
#include <QtQmlIntegration/qqmlintegration.h>

class Output
{
public:
    enum OutputType {
        Unknown,
        Laptop,
        Monitor,
        Television,
        Workspace,
        Virtual,
        Region,
    };

    Output(OutputType outputType, QScreen *screen, const QString &display, const QString &uniqueId, const QString &name)
        : m_outputType(outputType)
        , m_screen(screen)
        , m_display(display)
        , m_uniqueId(uniqueId)
        , m_name(name)
    {
    }

    QScreen *screen() const
    {
        return m_screen;
    }

    QString name() const
    {
        return m_name;
    }

    [[nodiscard]] QString iconName() const;
    [[nodiscard]] QString description() const;
    [[nodiscard]] bool isSynthetic() const;

    QString display() const
    {
        return m_display;
    }

    QString uniqueId() const
    {
        return m_uniqueId;
    }

    OutputType outputType() const
    {
        return m_outputType;
    }

private:
    OutputType m_outputType;
    QScreen *m_screen = nullptr;
    QString m_display;
    QString m_uniqueId;
    QString m_name;
};

class OutputsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("OutputsModel is passed in through the root properties")
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
public:
    enum Option {
        None = 0,
        WorkspaceIncluded = 0x1,
        VirtualIncluded = 0x2,
        RegionIncluded = 0x4,
        OutputsExcluded = 0x8
    };
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option)

    enum Roles {
        ScreenRole = Qt::UserRole,
        NameRole,
        IsSyntheticRole,
        DescriptionRole,
        GeometryRole,
    };
    Q_ENUM(Roles)

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

    static QString virtualScreenIdForApp(const QString &appId);

    /*
        \brief used for finding indexes based on whether they intersect with a given screen geometry. IOW: if the window is visible on that screen
        Mind that the call signature must be kept in sync with FilteredWindowModel! We invoke it on both the output and window model.
    */
    Q_INVOKABLE [[nodiscard]] bool geometryIntersects(const QModelIndex &index, const QRect &geometry) const
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
            qWarning() << "Invalid index for geometry intersection check:" << index;
            return false;
        }
        if (data(index, IsSyntheticRole).toBool()) {
            return false;
        }
        // An intersection is actually not called for here.
        // In theory two outputs can overlap partially, so let's not look for intersection but equality.
        return data(index, GeometryRole).toRect() == geometry;
    }

public Q_SLOTS:
    void clearSelection();

Q_SIGNALS:
    void hasSelectionChanged();

private:
    QList<Output> m_outputs;
    QSet<quint32> m_selectedRows;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OutputsModel::Options)
