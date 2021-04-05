// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KIOWidgets/KDirLister>
#include <QObject>

class DirLister : public KDirLister
{
    Q_OBJECT

public:
    explicit DirLister(QObject *parent = nullptr);

    void handleError(KIO::Job *job) override;
    void handleErrorMessage(const QString &message) override;

Q_SIGNALS:
    void errorOccured(const QString &message);
};
