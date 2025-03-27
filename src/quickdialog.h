/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QEventLoop>
#include <QObject>

class QWindow;

class QuickDialog : public QObject
{
    Q_OBJECT
public:
    QuickDialog(QObject *parent = nullptr);
    ~QuickDialog() override;

    enum class Result : unsigned {
        Accepted = 0,
        Rejected = 1,
    };

    QWindow *windowHandle() const
    {
        return m_theDialog;
    }

    void create(const QString &file, const QVariantMap &props);

public Q_SLOTS:
    void reject();
    virtual void accept();

Q_SIGNALS:
    void finished(Result result);
    void accepted();
    void rejected();

protected:
    QWindow *m_theDialog = nullptr;
};
