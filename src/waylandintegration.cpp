/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "waylandintegration.h"
#include "screencast.h"
#include "screencasting.h"
#include "wayland-wayland-client-protocol.h"
#include "waylandintegration_debug.h"
#include "waylandintegration_p.h"

#include <QDBusMetaType>
#include <QGuiApplication>

#include <KNotification>
#include <QEventLoop>
#include <QImage>
#include <QMenu>
#include <QThread>
#include <QTimer>
#include <QWaylandClientExtensionTemplate>

#include <KLocalizedString>

// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>

#include <waylandintegration_debug.h>

Q_GLOBAL_STATIC(WaylandIntegration::WaylandIntegrationPrivate, globalWaylandIntegration)

void WaylandIntegration::init()
{
    globalWaylandIntegration->initWayland();
}

bool WaylandIntegration::isStreamingAvailable()
{
    return globalWaylandIntegration->isStreamingAvailable();
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::startStreamingOutput(QScreen *screen, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingOutput(screen, mode);
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::startStreamingWorkspace(Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingWorkspace(mode);
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::startStreamingRegion(const QRect &region, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingRegion(region, mode);
}

std::unique_ptr<ScreencastingStream>
WaylandIntegration::startStreamingVirtual(const QString &name, const QString &description, const QSize &size, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingVirtualOutput(name, description, size, mode);
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::startStreamingWindow(KWayland::Client::PlasmaWindow *window, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingWindow(window, mode);
}

KWayland::Client::PlasmaWindowManagement *WaylandIntegration::plasmaWindowManagement()
{
    return globalWaylandIntegration->plasmaWindowManagement();
}

WaylandIntegration::WaylandIntegration *WaylandIntegration::waylandIntegration()
{
    return globalWaylandIntegration;
}

WaylandIntegration::WaylandIntegrationPrivate::WaylandIntegrationPrivate()
    : WaylandIntegration()
    , m_registryInitialized(false)
    , m_registry(nullptr)
    , m_screencasting(nullptr)
{
}

WaylandIntegration::WaylandIntegrationPrivate::~WaylandIntegrationPrivate() = default;

bool WaylandIntegration::WaylandIntegrationPrivate::isStreamingAvailable() const
{
    return m_screencasting;
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreamingWindow(KWayland::Client::PlasmaWindow *window,
                                                                                                         Screencasting::CursorMode cursorMode)
{
    return startStreaming(m_screencasting->createWindowStream(window, cursorMode));
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreamingOutput(QScreen *screen, Screencasting::CursorMode mode)
{
    auto stream = m_screencasting->createOutputStream(screen, mode);
    if (!stream) {
        qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "Cannot stream, output not found" << screen->name();
        auto notification = new KNotification(QStringLiteral("screencastfailure"), KNotification::CloseOnTimeout);
        notification->setTitle(i18n("Failed to start screencasting"));
        notification->setIconName(QStringLiteral("dialog-error"));
        notification->sendEvent();
        return {};
    }

    return startStreaming(stream);
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreamingWorkspace(Screencasting::CursorMode mode)
{
    const auto screens = qGuiApp->screens();
    const auto geometries = std::views::transform(screens, &QScreen::geometry);
    const QRect workspace = std::ranges::fold_left(geometries, QRect(), &QRect::united);
    return startStreaming(m_screencasting->createRegionStream(workspace, 1, mode));
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreamingRegion(const QRect region, Screencasting::CursorMode mode)
{
    return startStreaming(m_screencasting->createRegionStream(region, 0, mode));
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreamingVirtualOutput(const QString &name,
                                                                                                                const QString &description,
                                                                                                                const QSize &size,
                                                                                                                Screencasting::CursorMode mode)
{
    return startStreaming(m_screencasting->createVirtualOutputStream(name, description, size, 1, mode));
}

std::unique_ptr<ScreencastingStream> WaylandIntegration::WaylandIntegrationPrivate::startStreaming(ScreencastingStream *stream)
{
    QEventLoop loop;
    std::unique_ptr<ScreencastingStream> ret;

    connect(stream, &ScreencastingStream::failed, &loop, [&](const QString &error) {
        qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "failed to start streaming" << stream << error;

        KNotification *notification = new KNotification(QStringLiteral("screencastfailure"), KNotification::CloseOnTimeout);
        notification->setTitle(i18n("Failed to start screencasting"));
        notification->setText(error);
        notification->setIconName(QStringLiteral("dialog-error"));
        notification->sendEvent();

        loop.quit();
    });
    connect(stream, &ScreencastingStream::created, &loop, [&ret, &loop, stream] {
        ret.reset(stream);
        loop.quit();
    });
    QTimer::singleShot(3000, &loop, [&loop, stream] {
        stream->deleteLater();
        loop.quit();
    });
    loop.exec();
    return ret;
}

KWayland::Client::PlasmaWindowManagement *WaylandIntegration::WaylandIntegrationPrivate::plasmaWindowManagement()
{
    return m_windowManagement;
}

void WaylandIntegration::WaylandIntegrationPrivate::initWayland()
{
    auto connection = KWayland::Client::ConnectionThread::fromApplication(QGuiApplication::instance());

    if (!connection) {
        return;
    }

    m_registry = new KWayland::Client::Registry(this);

    connect(m_registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this](const QByteArray &interfaceName, quint32 name, quint32 version) {
        if (interfaceName != "zkde_screencast_unstable_v1")
            return;
        m_screencasting = new Screencasting(m_registry, name, version, this);
    });
    connect(m_registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, this, [this](quint32 name, quint32 version) {
        m_windowManagement = m_registry->createPlasmaWindowManagement(name, version, this);
        Q_EMIT waylandIntegration()->plasmaWindowManagementInitialized();
    });
    connect(m_registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Registry initialized";
    });

    m_registry->create(connection);
    m_registry->setup();
}

#include "moc_waylandintegration.cpp"
#include "moc_waylandintegration_p.cpp"
