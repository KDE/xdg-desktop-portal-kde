/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "remotedesktop.h"
#include "remotedesktop_debug.h"
#include "remotedesktopdialog.h"
#include "request.h"
#include "session.h"
#include "utils.h"
#include "waylandintegration.h"
#include <KLocalizedString>
#include <KNotification>
#include <QGuiApplication>
#include <QRegion>
#include <QScreen>

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
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    Session *session = Session::createSession(this, Session::RemoteDesktop, app_id, session_handle.path());

    if (!session) {
        return 2;
    }

    if (!WaylandIntegration::isStreamingAvailable()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "zkde_screencast_unstable_v1 does not seem to be available";
        return 2;
    }

    return 0;
}

uint RemoteDesktopPortal::SelectDevices(const QDBusObjectPath &handle,
                                        const QDBusObjectPath &session_handle,
                                        const QString &app_id,
                                        const QVariantMap &options,
                                        QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "SelectDevices called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    const auto types = static_cast<RemoteDesktopPortal::DeviceTypes>(options.value(QStringLiteral("types")).toUInt());
    if (types == None) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "There are no devices to remotely control";
        return 2;
    }

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to select sources on non-existing session " << session_handle.path();
        return 2;
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

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call start on non-existing session " << session_handle.path();
        return 2;
    }

    if (WaylandIntegration::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Failed to show dialog as there is no screen to select";
        return 2;
    }

    if (app_id.isEmpty()) {
        // non-sandboxed local applications are generally able to take over the system already without much hassle.
        // Instead of making users interact with the dialog (which they probably can't because this is for remote access), just
        // show a notification so they are aware of it.
        auto notification = new KNotification(QStringLiteral("remotedesktopstarted"), KNotification::CloseOnTimeout);
        notification->setTitle(i18nc("title of notification about input systems taken over", "Remote control session started"));
        notification->setText(RemoteDesktopDialog::buildDescription(app_id, session->deviceTypes(), session->screenSharingEnabled()));
        notification->setIconName(QStringLiteral("krfb"));
        notification->sendEvent();
    } else {
        QScopedPointer<RemoteDesktopDialog, QScopedPointerDeleteLater> remoteDesktopDialog(
            new RemoteDesktopDialog(app_id, session->deviceTypes(), session->screenSharingEnabled()));
        Utils::setParentWindow(remoteDesktopDialog->windowHandle(), parent_window);
        Request::makeClosableDialogRequest(handle, remoteDesktopDialog.get());
        connect(session, &Session::closed, remoteDesktopDialog.data(), &RemoteDesktopDialog::reject);

        if (!remoteDesktopDialog->exec()) {
            return 1;
        }
    }

    if (session->screenSharingEnabled()) {
        WaylandIntegration::Streams streams;
        const auto screens = qGuiApp->screens();
        if (session->multipleSources() || screens.count() == 1) {
            for (const auto &screen : screens) {
                auto stream = WaylandIntegration::startStreamingOutput(screen, Screencasting::Metadata);
                if (!stream.isValid()) {
                    return 2;
                }
                streams << stream;
            }
        } else {
            streams << WaylandIntegration::startStreamingWorkspace(Screencasting::Metadata);
        }

        session->setStreams(streams);
        results.insert(QStringLiteral("streams"), QVariant::fromValue<WaylandIntegration::Streams>(streams));
    } else {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop()) << "Only stream input";
        session->refreshDescription();
    }
    session->acquireStreamingInput();

    results.insert(QStringLiteral("devices"), QVariant::fromValue<uint>(session->deviceTypes()));

    return 0;
}

void RemoteDesktopPortal::NotifyPointerMotion(const QDBusObjectPath &session_handle, const QVariantMap &options, double dx, double dy)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerMotion called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dx: " << dx;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dy: " << dy;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerMotion on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerMotion(QSizeF(dx, dy));
}

void RemoteDesktopPortal::NotifyPointerMotionAbsolute(const QDBusObjectPath &session_handle, const QVariantMap &options, uint stream, double x, double y)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerMotionAbsolute called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    stream: " << stream;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    x: " << x;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    y: " << y;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerMotionAbsolute on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerMotionAbsolute(QPointF(x, y));
}

void RemoteDesktopPortal::NotifyPointerButton(const QDBusObjectPath &session_handle, const QVariantMap &options, int button, uint state)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerButton called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    button: " << button;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    state: " << state;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

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

void RemoteDesktopPortal::NotifyPointerAxis(const QDBusObjectPath &session_handle, const QVariantMap &options, double dx, double dy)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerAxis called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dx: " << dx;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dy: " << dy;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyKeyboardKeysym on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerAxis(dx, dy);
}

void RemoteDesktopPortal::NotifyPointerAxisDiscrete(const QDBusObjectPath &session_handle, const QVariantMap &options, uint axis, int steps)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerAxisDiscrete called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    axis: " << axis;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    steps: " << steps;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerAxisDiscrete(!axis ? Qt::Vertical : Qt::Horizontal, steps);
}

void RemoteDesktopPortal::NotifyKeyboardKeysym(const QDBusObjectPath &session_handle, const QVariantMap &options, int keysym, uint state)
{
    Q_UNUSED(options)

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyKeyboardKeysym on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestKeyboardKeysym(keysym, state != 0);
}

void RemoteDesktopPortal::NotifyKeyboardKeycode(const QDBusObjectPath &session_handle, const QVariantMap &options, int keycode, uint state)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyKeyboardKeycode called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    keycode: " << keycode;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    state: " << state;

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyKeyboardKeycode on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestKeyboardKeycode(keycode, state != 0);
}

void RemoteDesktopPortal::NotifyTouchDown(const QDBusObjectPath &session_handle, const QVariantMap &options, uint stream, uint slot, int x, int y)
{
    Q_UNUSED(options)
    Q_UNUSED(stream)

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }
    WaylandIntegration::requestTouchDown(slot, QPoint(x, y));
}

void RemoteDesktopPortal::NotifyTouchMotion(const QDBusObjectPath &session_handle, const QVariantMap &options, uint stream, uint slot, int x, int y)
{
    Q_UNUSED(options)
    Q_UNUSED(stream)

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }
    WaylandIntegration::requestTouchMotion(slot, QPoint(x, y));
}

void RemoteDesktopPortal::NotifyTouchUp(const QDBusObjectPath &session_handle, const QVariantMap &options, uint slot)
{
    Q_UNUSED(options)

    RemoteDesktopSession *session = qobject_cast<RemoteDesktopSession *>(Session::getSession(session_handle.path()));
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestTouchUp(slot);
}
