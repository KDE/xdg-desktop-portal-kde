/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 * SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_REQUEST_H
#define XDG_DESKTOP_PORTAL_KDE_REQUEST_H

#include "session.h"

#include <QDBusVirtualObject>
#include <QObject>

class QDBusObjectPath;

class Request : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Request")
    Q_PROPERTY(uint version MEMBER version CONSTANT)
public:
    explicit Request(const QDBusObjectPath &handle, QObject *parent = nullptr);

    static constexpr uint version = 2;

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

Q_SIGNALS:
    void closeRequested();
    Q_SCRIPTABLE void Created();

public Q_SLOTS:
    Q_SCRIPTABLE void Close()
    {
        Q_EMIT closeRequested();
    };
};

#endif // XDG_DESKTOP_PORTAL_KDE_REQUEST_H
