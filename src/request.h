/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_REQUEST_H
#define XDG_DESKTOP_PORTAL_KDE_REQUEST_H

#include "session.h"

#include <QObject>

class QDBusObjectPath;

class Request : public QObject
{
    Q_OBJECT
public:
    explicit Request(const QDBusObjectPath &handle, QObject *parent = nullptr, const QString &portalName = QString(), const QVariant &data = QVariant());

    template<class T>
    static Request *makeClosableDialogRequest(const QDBusObjectPath &handle, T *dialogAndParent)
    {
        auto request = new Request(handle, dialogAndParent);
        connect(request, &Request::closeRequested, dialogAndParent, &T::reject);
        return request;
    }

    template<class T>
    static Request *makeClosableDialogRequestWithSession(const QDBusObjectPath &handle, T *dialogAndParent, Session *session)
    {
        auto request = makeClosableDialogRequest(handle, dialogAndParent);
        connect(session, &Session::closed, dialogAndParent, &T::reject);
        return request;
    }

public Q_SLOTS:
    void close();

Q_SIGNALS:
    void closeRequested();

private:
    const QVariant m_data;
    const QString m_portalName;
    const QDBusObjectPath m_path;
};

#endif // XDG_DESKTOP_PORTAL_KDE_REQUEST_H
