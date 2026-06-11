/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
#define XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H

#include <QDBusArgument>
#include <QObject>
#include <QPoint>
#include <QScreen>
#include <QSize>
#include <QVariant>

#include <KWayland/Client/output.h>
#include <screencasting.h>

namespace KWayland
{
namespace Client
{
class PlasmaWindowManagement;
}
}

class QWindow;

namespace WaylandIntegration
{

class WaylandIntegration : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void plasmaWindowManagementInitialized();
};

bool isStreamingAvailable();

std::unique_ptr<ScreencastingStream> startStreamingOutput(QScreen *screen, Screencasting::CursorMode mode);
std::unique_ptr<ScreencastingStream> startStreamingWorkspace(Screencasting::CursorMode mode);
std::unique_ptr<ScreencastingStream> startStreamingVirtual(const QString &name, const QString &description, const QSize &size, Screencasting::CursorMode mode);
std::unique_ptr<ScreencastingStream> startStreamingWindow(KWayland::Client::PlasmaWindow *window, Screencasting::CursorMode mode);
std::unique_ptr<ScreencastingStream> startStreamingRegion(const QRect &region, Screencasting::CursorMode mode);

void init();

KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

WaylandIntegration *waylandIntegration();
}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
