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

#include "inputcapture.h"

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

class InputCaptureSession : public Session
{
    Q_OBJECT
public:
    explicit InputCaptureSession(QObject *parent, const QString &appId, const QString &path);
    ~InputCaptureSession() override;

    SessionType type() const override
    {
        return SessionType::InputCapture;
    }

    InputCapturePortal::State state;

    void connect(const QDBusObjectPath &path);
    QDBusObjectPath kwinInputCapture() const;

    QDBusPendingReply<void> enable();
    QDBusPendingReply<void> disable();
    QDBusPendingReply<void> release(const QPointF &cusorPosition, bool applyPosition);

    QDBusPendingReply<QDBusUnixFileDescriptor> connectToEIS();

    void addBarrier(const QPair<QPoint, QPoint> &barriers);
    void clearBarriers();

Q_SIGNALS:
    void disabled();
    void activated(uint activationId, const QPointF &cursorPosition);
    void deactivated(uint activationId);

private:
    QDBusObjectPath m_kwinInputCapture;
    QList<QPair<QPoint, QPoint>> m_barriers;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SESSION_H
