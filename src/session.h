/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SESSION_H
#define XDG_DESKTOP_PORTAL_KDE_SESSION_H

#include <QAction>
#include <QDBusPendingReply>
#include <QObject>
#include <QShortcut>

class KStatusNotifierItem;
class KGlobalAccelInterface;
class KGlobalAccelComponentInterface;
class RemoteDesktopPortal;

class Session : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Session)
public:
    explicit Session(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~Session() override;

    enum SessionType {
        ScreenCast = 0,
        RemoteDesktop = 1,
        GlobalShortcuts = 2,
        InputCapture = 3,
    };

    bool close();
    virtual SessionType type() const = 0;

    static Session *getSession(const QString &sessionHandle);
    template<typename T>
    static T *getSession(const QString &sessionHandle)
    {
        return qobject_cast<T *>(getSession(sessionHandle));
    }

    /*
     * The path of the session
     */
    QString handle() const
    {
        return m_path;
    }

    QString appId() const
    {
        return m_appId;
    }

    /*
     * Internal: For DBus consumption
     */
    uint version() const
    {
        return 2;
    }

    /*
     * Returns if the Session was registered successfully
     */
    bool isValid() const
    {
        return m_valid;
    }

    /*
     * Internal: For DBus consumption
     */
    void Close();

Q_SIGNALS:
    void closed();

protected:
    const QString m_appId;
    const QString m_path;

private:
    bool m_valid = false;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SESSION_H
