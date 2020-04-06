/*
 * Copyright © 2018 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "remotedesktop.h"
#include "session.h"
#include "remotedesktopdialog.h"
#include "utils.h"
#include "waylandintegration.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeRemoteDesktop, "xdp-kde-remotedesktop")

RemoteDesktopPortal::RemoteDesktopPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

RemoteDesktopPortal::~RemoteDesktopPortal()
{
}

uint RemoteDesktopPortal::CreateSession(const QDBusObjectPath &handle,
                                        const QDBusObjectPath &session_handle,
                                        const QString &app_id,
                                        const QVariantMap &options,
                                        QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    Session *session = Session::createSession(this, Session::RemoteDesktop, app_id, session_handle.path());

    if (!session) {
        return 2;
    }

    connect(session, &Session::closed, [this] () {
        WaylandIntegration::stopStreaming();
    });

    return 0;
}

uint RemoteDesktopPortal::SelectDevices(const QDBusObjectPath &handle,
                                        const QDBusObjectPath &session_handle,
                                        const QString &app_id,
                                        const QVariantMap &options,
                                        QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "SelectDevices called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    RemoteDesktopPortal::DeviceTypes types = RemoteDesktopPortal::None;
    if (options.contains(QStringLiteral("types"))) {
        types = static_cast<RemoteDesktopPortal::DeviceTypes>(options.value(QStringLiteral("types")).toUInt());
    }

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to select sources on non-existing session " << session_handle.path();
        return 2;
    }

    if (options.contains(QStringLiteral("types"))) {
        types = (DeviceTypes)(options.value(QStringLiteral("types")).toUInt());
    }
    session->setDeviceTypes(types);

    return 0;
}

uint RemoteDesktopPortal::Start(const QDBusObjectPath &handle,
                                const QDBusObjectPath &session_handle,
                                const QString &app_id,
                                const QString &parent_window,
                                const QVariantMap &options,
                                QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "Start called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call start on non-existing session " << session_handle.path();
        return 2;
    }

    // TODO check whether we got some outputs?
    if (WaylandIntegration::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Failed to show dialog as there is no screen to select";
        return 2;
    }

    QScopedPointer<RemoteDesktopDialog, QScopedPointerDeleteLater> remoteDesktopDialog(new RemoteDesktopDialog(app_id, session->deviceTypes(), session->screenSharingEnabled(), session->multipleSources()));
    Utils::setParentWindow(remoteDesktopDialog.data(), parent_window);

    connect(session, &Session::closed, remoteDesktopDialog.data(), &RemoteDesktopDialog::reject);

    if (remoteDesktopDialog->exec()) {
        if (session->screenSharingEnabled()) {
            if (!WaylandIntegration::startStreaming(remoteDesktopDialog->selectedScreens().first())) {
                return 2;
            }

            WaylandIntegration::authenticate();

            QVariant streams = WaylandIntegration::streams();

            if (!streams.isValid()) {
                qCWarning(XdgDesktopPortalKdeRemoteDesktop()) << "Pipewire stream is not ready to be streamed";
                return 2;
            }

            results.insert(QStringLiteral("streams"), streams);
        } else {
            qCWarning(XdgDesktopPortalKdeRemoteDesktop()) << "Only stream input";
            WaylandIntegration::startStreamingInput();
            WaylandIntegration::authenticate();
        }

        results.insert(QStringLiteral("devices"), QVariant::fromValue<uint>(remoteDesktopDialog->deviceTypes()));

        return 0;
    }

    return 1;
}

void RemoteDesktopPortal::NotifyPointerMotion(const QDBusObjectPath &session_handle,
                                              const QVariantMap &options,
                                              double dx,
                                              double dy)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerMotion called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dx: " << dx;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dy: " << dy;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerMotion on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerMotion(QSizeF(dx, dy));
}

void RemoteDesktopPortal::NotifyPointerMotionAbsolute(const QDBusObjectPath &session_handle,
                                                      const QVariantMap &options,
                                                      uint stream,
                                                      double x,
                                                      double y)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerMotionAbsolute called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    stream: " << stream;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    x: " << x;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    y: " << y;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerMotionAbsolute on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerMotionAbsolute(QPointF(x, y));
}

void RemoteDesktopPortal::NotifyPointerButton(const QDBusObjectPath &session_handle,
                                              const QVariantMap &options,
                                              int button,
                                              uint state)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerButton called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    button: " << button;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    state: " << state;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerButton on non-existing session " << session_handle.path();
        return;
    }

    if (state) {
        WaylandIntegration::requestPointerButtonPress(button);
    } else {
        WaylandIntegration::requestPointerButtonRelease(button);
    }
}

void RemoteDesktopPortal::NotifyPointerAxis(const QDBusObjectPath &session_handle,
                                            const QVariantMap &options,
                                            double dx,
                                            double dy)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerAxis called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dx: " << dx;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dy: " << dy;
}

void RemoteDesktopPortal::NotifyPointerAxisDiscrete(const QDBusObjectPath &session_handle,
                                                    const QVariantMap &options,
                                                    uint axis,
                                                    int steps)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerAxisDiscrete called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    axis: " << axis;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    steps: " << steps;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerAxisDiscrete(!axis ? Qt::Vertical : Qt::Horizontal, steps);
}

void RemoteDesktopPortal::NotifyKeyboardKeysym(const QDBusObjectPath &session_handle,
                                               const QVariantMap &options,
                                               int keysym,
                                               uint state)
{
}

void RemoteDesktopPortal::NotifyKeyboardKeycode(const QDBusObjectPath &session_handle,
                                                const QVariantMap &options,
                                                int keycode,
                                                uint state)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyKeyboardKeycode called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    keycode: " << keycode;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    state: " << state;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyKeyboardKeycode on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestKeyboardKeycode(keycode, state != 0);
}

void RemoteDesktopPortal::NotifyTouchDown(const QDBusObjectPath &session_handle,
                                          const QVariantMap &options,
                                          uint stream,
                                          uint slot,
                                          int x,
                                          int y)
{
}

void RemoteDesktopPortal::NotifyTouchMotion(const QDBusObjectPath &session_handle,
                                            const QVariantMap &options,
                                            uint stream,
                                            uint slot,
                                            int x,
                                            int y)
{
}

void RemoteDesktopPortal::NotifyTouchUp(const QDBusObjectPath &session_handle,
                                        const QVariantMap &options,
                                        uint slot)
{
}
