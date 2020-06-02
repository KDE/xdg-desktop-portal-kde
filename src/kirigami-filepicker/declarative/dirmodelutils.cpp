// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "dirmodelutils.h"

#include <QUrl>
#include <QStandardPaths>

#include <KIO/MkdirJob>

DirModelUtils::DirModelUtils(QObject *parent) : QObject(parent)
{
}

QStringList DirModelUtils::getUrlParts(const QUrl &url) const
{
    if (url.path() == QStringLiteral("/"))
        return {};

    return url.path().split(QStringLiteral("/")).mid(1);
}

QUrl DirModelUtils::partialUrlForIndex(QUrl url, int index) const
{
    const QStringList urlParts = url.path().split(QStringLiteral("/"));
    QString path = QStringLiteral("/");
    for (int i = 0; i < index + 1; i++) {
        path += urlParts.at(i + 1);
        path += QStringLiteral("/");
    }

    url.setPath(path);

    return url;
}

QString DirModelUtils::homePath() const
{
    QUrl url(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    url.setScheme(QStringLiteral("file"));

    return url.toString();
}

void DirModelUtils::mkdir(const QUrl path) const
{
    KIO::mkdir(path);
}
