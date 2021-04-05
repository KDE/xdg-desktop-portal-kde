// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "dirlister.h"

#include <KIO/Job>

DirLister::DirLister(QObject *parent)
    : KDirLister(parent)
{
}

void DirLister::handleError(KIO::Job *job)
{
    handleErrorMessage(job->errorString());
}

void DirLister::handleErrorMessage(const QString &message)
{
    Q_EMIT errorOccured(message);
}
