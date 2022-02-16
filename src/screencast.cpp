/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screencast.h"
#include "notificationinhibition.h"
#include "screenchooserdialog.h"
#include "session.h"
#include "utils.h"
#include "waylandintegration.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QLoggingCategory>
#include <QWindow>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenCast, "xdp-kde-screencast")

ScreenCastPortal::ScreenCastPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

ScreenCastPortal::~ScreenCastPortal()
{
}

bool ScreenCastPortal::inhibitionsEnabled() const
{
    if (!WaylandIntegration::isStreamingAvailable()) {
        return false;
    }

    auto cfg = KSharedConfig::openConfig(QStringLiteral("plasmanotifyrc"));

    KConfigGroup grp(cfg, "DoNotDisturb");

    return grp.readEntry("WhenScreenSharing", true);
}

uint ScreenCastPortal::CreateSession(const QDBusObjectPath &handle,
                                     const QDBusObjectPath &session_handle,
                                     const QString &app_id,
                                     const QVariantMap &options,
                                     QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeScreenCast) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    Session *session = Session::createSession(this, Session::ScreenCast, app_id, session_handle.path());

    if (!session) {
        return 2;
    }

    if (!WaylandIntegration::isStreamingAvailable()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "zkde_screencast_unstable_v1 does not seem to be available";
        return 2;
    }

    connect(session, &Session::closed, []() {
        WaylandIntegration::stopAllStreaming();
    });

    connect(WaylandIntegration::waylandIntegration(), &WaylandIntegration::WaylandIntegration::streamingStopped, session, &Session::close);

    return 0;
}

uint ScreenCastPortal::SelectSources(const QDBusObjectPath &handle,
                                     const QDBusObjectPath &session_handle,
                                     const QString &app_id,
                                     const QVariantMap &options,
                                     QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeScreenCast) << "SelectSource called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    ScreenCastSession *session = qobject_cast<ScreenCastSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to select sources on non-existing session " << session_handle.path();
        return 2;
    }

    session->setOptions(options);

    // Might be also a RemoteDesktopSession
    if (session->type() == Session::RemoteDesktop) {
        RemoteDesktopSession *remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(session);
        if (remoteDesktopSession) {
            remoteDesktopSession->setScreenSharingEnabled(true);
        }
    }

    return 0;
}

uint ScreenCastPortal::Start(const QDBusObjectPath &handle,
                             const QDBusObjectPath &session_handle,
                             const QString &app_id,
                             const QString &parent_window,
                             const QVariantMap &options,
                             QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeScreenCast) << "Start called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    ScreenCastSession *session = qobject_cast<ScreenCastSession *>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to call start on non-existing session " << session_handle.path();
        return 2;
    }

    if (WaylandIntegration::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Failed to show dialog as there is no screen to select";
        return 2;
    }

    QScopedPointer<ScreenChooserDialog, QScopedPointerDeleteLater> screenDialog(
        new ScreenChooserDialog(app_id, session->multipleSources(), SourceTypes(session->types())));
    screenDialog->windowHandle()->show();
    Utils::setParentWindow(screenDialog->windowHandle(), parent_window);

    connect(session, &Session::closed, screenDialog.data(), &ScreenChooserDialog::reject);

    if (screenDialog->exec()) {
        const auto selectedScreens = screenDialog->selectedScreens();
        for (quint32 outputid : selectedScreens) {
            if (!WaylandIntegration::startStreamingOutput(outputid, Screencasting::CursorMode(session->cursorMode()))) {
                return 2;
            }
        }
        const auto selectedWindows = screenDialog->selectedWindows();
        for (const auto &win : selectedWindows) {
            if (!WaylandIntegration::startStreamingWindow(win)) {
                return 2;
            }
        }

        QVariant streams = WaylandIntegration::streams();

        if (!streams.isValid()) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "Pipewire stream is not ready to be streamed";
            return 2;
        }

        results.insert(QStringLiteral("streams"), streams);

        if (inhibitionsEnabled()) {
            new NotificationInhibition(app_id, i18nc("Do not disturb mode is enabled because...", "Screen sharing in progress"), session);
        }
        return 0;
    }

    return 1;
}
