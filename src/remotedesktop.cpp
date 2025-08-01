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
#include "restoredata.h"
#include "session.h"
#include "utils.h"
#include "waylandintegration.h"
#include <KLocalizedString>
#include <KNotification>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QGuiApplication>
#include <QRegion>
#include <QScreen>

#include "permission_store.h"

using namespace Qt::StringLiterals;

namespace
{

// This is a helper function to check if the application is mega-authorized.
// Mega-authorization is a permission system specific to the KDE portal implementation.
// A mega-authorized application is one that has been granted permissions to access all (remote desktop) features **without** any further user interaction.
// This function should be used to check if a UI interaction can be skipped right away.
// For instance if an application is mega-authorized we don't need to ask the user if they want to allow keyboard/mouse input. It's always authorized.
// Particularly useful for headless setups and when the user is not physically at the machine.
bool isAppMegaAuthorized(const QString &app_id)
{
    // NOTE: an empty app_id should never occur for flatpak/snap applications and as such is meant to denote a host application
    //   of which the app_id is not known. In such a case the user may authorize the empty app_id to cover generic host applications.
    //   Specifically xwayland may request input permissions but has no app_id.
    //   Note that this is different from giving out an "any" permission. An application that has an app_id will not be covered by the empty rule.
    qDBusRegisterMetaType<AppIdPermissionsMap>();
    OrgFreedesktopImplPortalPermissionStoreInterface permissionStore(u"org.freedesktop.impl.portal.PermissionStore"_s,
                                                                     u"/org/freedesktop/impl/portal/PermissionStore"_s,
                                                                     QDBusConnection::sessionBus());
    // Bring the timeout way down. Permission store queries are fast, if they aren't then something is wrong and there is no point waiting a long time.
    permissionStore.setTimeout(1000);
    QDBusVariant data;
    auto reply = permissionStore.Lookup(u"kde-authorized"_s, u"remote-desktop"_s, data);
    if (reply.isValid()) {
        auto appIdPermissions = reply.value();
        if (!appIdPermissions.contains(app_id)) {
            qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "MegaAuth: Permission not granted for" << app_id;
            return false;
        }

        auto permissions = appIdPermissions.value(app_id);
        if (permissions.contains("yes"_L1)) {
            qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "MegaAuth: Permission granted for" << app_id;
            return true;
        }
    } else {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "MegaAuth: Failed to lookup permissions:" << reply.error().message();
    }

    return false;
};

} // namespace

static QString kwinService()
{
    return QStringLiteral("org.kde.KWin");
}

static QString kwinRemoteDesktopPath()
{
    return QStringLiteral("/org/kde/KWin/EIS/RemoteDesktop");
}

static QString kwinRemoteDesktopInterface()
{
    return QStringLiteral("org.kde.KWin.EIS.RemoteDesktop");
}

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
        return PortalResponse::OtherError;
    }

    if (!WaylandIntegration::isStreamingAvailable()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "zkde_screencast_unstable_v1 does not seem to be available";
        return PortalResponse::OtherError;
    }

    connect(session, &Session::closed, [session] {
        auto remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(session);
        const auto streams = remoteDesktopSession->streams();
        for (const WaylandIntegration::Stream &stream : streams) {
            WaylandIntegration::stopStreaming(stream.nodeId);
        }
        if (remoteDesktopSession->eisCookie()) {
            auto msg = QDBusMessage::createMethodCall(kwinService(), kwinRemoteDesktopPath(), kwinRemoteDesktopInterface(), QStringLiteral("disconnect"));
            msg.setArguments({remoteDesktopSession->eisCookie()});
            QDBusConnection::sessionBus().send(msg);
        }
    });

    return PortalResponse::Success;
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
        return PortalResponse::OtherError;
    }

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to select sources on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    session->setDeviceTypes(types);
    session->setPersistMode(ScreenCastPortal::PersistMode(options.value(QStringLiteral("persist_mode")).toUInt()));
    session->setRestoreData(options.value(QStringLiteral("restore_data")));

    return PortalResponse::Success;
}

std::pair<PortalResponse::Response, QVariantMap> continueStart(RemoteDesktopSession *session, ScreenCastPortal::PersistMode persist)
{
    QVariantMap results;
    QPointer<RemoteDesktopSession> guardedSession(session);
    if (session->screenSharingEnabled()) {
        WaylandIntegration::Streams streams;
        if (session->types() == ScreenCastPortal::Virtual) {
            const QString outputName = session->appId().isEmpty()
                ? i18n("Virtual Output")
                : i18nc("%1 is the application name", "Virtual Output (shared with %1)", Utils::applicationName(session->appId()));
            auto stream = WaylandIntegration::startStreamingVirtual(OutputsModel::virtualScreenIdForApp(session->appId()), outputName, {1920, 1080}, Screencasting::Metadata);
            if (!stream.isValid()) {
                return {PortalResponse::OtherError, {}};
            }
            streams << stream;
        } else {
            const auto screens = qGuiApp->screens();
            if (session->multipleSources() || screens.count() == 1) {
                for (const auto &screen : screens) {
                    auto stream = WaylandIntegration::startStreamingOutput(screen, Screencasting::Metadata);
                    if (!stream.isValid()) {
                        return {PortalResponse::OtherError, {}};
                    }
                    streams << stream;
                }
            } else {
                streams << WaylandIntegration::startStreamingWorkspace(Screencasting::Metadata);
            }
        }

        if (!guardedSession) {
            return {PortalResponse::OtherError, {}};
        }

        session->setStreams(streams);
        results.insert(QStringLiteral("streams"), QVariant::fromValue<WaylandIntegration::Streams>(streams));
    } else {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop()) << "Only stream input";
        session->refreshDescription();
    }
    session->acquireStreamingInput();

    results.insert(QStringLiteral("devices"), QVariant::fromValue<uint>(session->deviceTypes()));
    results.insert(QStringLiteral("clipboard_enabled"), false);
    if (session->persistMode() != ScreenCastPortal::NoPersist) {
        results.insert(u"persist_mode"_s, quint32(persist));
        if (persist != ScreenCastPortal::NoPersist) {
            const RestoreData restoreData = {u"KDE"_s,
                                             RestoreData::currentRestoreDataVersion(),
                                             QVariantMap{{u"screenShareEnabled"_s, session->screenSharingEnabled()},
                                                         {u"devices"_s, static_cast<quint32>(session->deviceTypes())},
                                                         {u"clipboardEnabled"_s, session->clipboardEnabled()}}};
            results.insert(u"restore_data"_s, QVariant::fromValue<RestoreData>(restoreData));
        }
    }
    return {PortalResponse::Success, results};
}

void RemoteDesktopPortal::Start(const QDBusObjectPath &handle,
                                const QDBusObjectPath &session_handle,
                                const QString &app_id,
                                const QString &parent_window,
                                const QVariantMap &options,
                                const QDBusMessage &message,
                                uint &replyResponse,
                                QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "Start called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call start on non-existing session " << session_handle.path();
        replyResponse = PortalResponse::OtherError;
        return;
    }

    if (QGuiApplication::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Failed to show dialog as there is no screen to select";
        replyResponse = PortalResponse::OtherError;
        return;
    }

    const ScreenCastPortal::PersistMode persist = session->persistMode();
    QList<Output> selectedOutputs;

    bool restored = false;

    if (persist != ScreenCastPortal::NoPersist && session->restoreData().isValid()) {
        const RestoreData restoreData = qdbus_cast<RestoreData>(session->restoreData().value<QDBusArgument>());
        if (restoreData.session == QLatin1String("KDE") && restoreData.version == RestoreData::currentRestoreDataVersion()) {
            // check we asked for the same key content both times; if not, don't restore
            // some settings (like ScreenCast multipleSources or cursorMode) don't involve user prompts so use whatever was explicitly
            // requested this time
            const RemoteDesktopPortal::DeviceTypes devices = static_cast<RemoteDesktopPortal::DeviceTypes>(restoreData.payload[u"devices"_s].toUInt());
            if (session->deviceTypes() != devices) {
                qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "Not restoring session as requested devices don't match";
            } else if (session->screenSharingEnabled() != restoreData.payload[u"screenShareEnabled"_s].toBool()) {
                qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "Not restoring session as requested screen sharing doesn't match";
            } else if (session->clipboardEnabled() != restoreData.payload[u"clipboardEnabled"_s].toBool()) {
                qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "Not restoring session as clipboard enabled doesn't match";
            } else {
                restored = true;
            }
        }
    }

    if (restored) {
        auto notification = new KNotification(QStringLiteral("remotedesktopstarted"), KNotification::CloseOnTimeout);
        notification->setTitle(i18nc("title of notification about input systems taken over", "Remote control session started"));
        notification->setText(RemoteDesktopDialog::buildDescription(app_id, session->deviceTypes(), session->screenSharingEnabled()));
        notification->setIconName(QStringLiteral("krfb"));
        notification->sendEvent();
    } else {
        if (!isAppMegaAuthorized(app_id)) { // authorize right away
            auto remoteDesktopDialog = new RemoteDesktopDialog(app_id, session->deviceTypes(), session->screenSharingEnabled(), session->persistMode());
            Utils::setParentWindow(remoteDesktopDialog->windowHandle(), parent_window);
            Request::makeClosableDialogRequestWithSession(handle, remoteDesktopDialog, session);
            delayReply(message, remoteDesktopDialog, this, [session, persist](DialogResult dialogResult) {
                auto response = PortalResponse::fromDialogResult(dialogResult);
                QVariantMap results;
                if (dialogResult == DialogResult::Accepted) {
                    std::tie(response, results) = continueStart(session, persist);
                }
                return QVariantList{response, results};
            });
            return;
        }
    }

    std::tie(replyResponse, replyResults) = continueStart(session, persist);
}

void RemoteDesktopPortal::NotifyPointerMotion(const QDBusObjectPath &session_handle, const QVariantMap &options, double dx, double dy)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerMotion called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dx: " << dx;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    dy: " << dy;

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerMotionAbsolute on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerMotionAbsolute(stream, QPointF(x, y));
}

void RemoteDesktopPortal::NotifyPointerButton(const QDBusObjectPath &session_handle, const QVariantMap &options, int button, uint state)
{
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "NotifyPointerButton called with parameters:";
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    button: " << button;
    qCDebug(XdgDesktopPortalKdeRemoteDesktop) << "    state: " << state;

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestPointerAxisDiscrete(!axis ? Qt::Vertical : Qt::Horizontal, steps);
}

void RemoteDesktopPortal::NotifyKeyboardKeysym(const QDBusObjectPath &session_handle, const QVariantMap &options, int keysym, uint state)
{
    Q_UNUSED(options)

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());

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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());
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

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }
    WaylandIntegration::requestTouchMotion(slot, QPoint(x, y));
}

void RemoteDesktopPortal::NotifyTouchUp(const QDBusObjectPath &session_handle, const QVariantMap &options, uint slot)
{
    Q_UNUSED(options)

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call NotifyPointerAxisDiscrete on non-existing session " << session_handle.path();
        return;
    }

    WaylandIntegration::requestTouchUp(slot);
}

QDBusUnixFileDescriptor
RemoteDesktopPortal::ConnectToEIS(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options, const QDBusMessage &message)
{
    Q_UNUSED(options)
    Q_UNUSED(app_id)

    RemoteDesktopSession *session = Session::getSession<RemoteDesktopSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Tried to call ConnectToEis on non-existing session " << session_handle.path();
        return QDBusUnixFileDescriptor();
    }

    auto msg = QDBusMessage::createMethodCall(kwinService(), kwinRemoteDesktopPath(), kwinRemoteDesktopInterface(), QStringLiteral("connectToEIS"));
    msg.setArguments({static_cast<int>(session->deviceTypes())});
    // Using pending reply for multiple return values
    QDBusPendingReply<QDBusUnixFileDescriptor, int> reply = QDBusConnection::sessionBus().call(msg);
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Failed to connect to EIS:" << reply.error();
        auto error = message.createErrorReply(QDBusError::Failed, QStringLiteral("Failed to connect to to EIS"));
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }
    session->setEisCookie(reply.argumentAt<1>());
    return reply.argumentAt<0>();
}
