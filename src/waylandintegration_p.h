/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H
#define XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H

#include "waylandintegration.h"

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QWaylandClientExtension>

#include "qwayland-fake-input.h"

class Screencasting;
class ScreencastingStream;

namespace KWayland
{
namespace Client
{
class ConnectionThread;
class EventQueue;
class Registry;
class PlasmaWindow;
class PlasmaWindowManagement;
class RemoteBuffer;
}
}

class QWindow;

namespace WaylandIntegration
{

class WaylandIntegrationPrivate : public WaylandIntegration::WaylandIntegration
{
    Q_OBJECT
public:
    WaylandIntegrationPrivate();
    ~WaylandIntegrationPrivate() override;

    void initWayland();

    KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

private:
    bool m_registryInitialized = false;

    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;

public:
    void authenticate();

    bool isStreamingAvailable() const;

    void acquireStreamingInput(bool acquire);

    std::unique_ptr<ScreencastingStream> startStreamingOutput(QScreen *screen, Screencasting::CursorMode mode);
    std::unique_ptr<ScreencastingStream> startStreamingWindow(KWayland::Client::PlasmaWindow *window, Screencasting::CursorMode mode);
    std::unique_ptr<ScreencastingStream> startStreamingWorkspace(Screencasting::CursorMode mode);
    std::unique_ptr<ScreencastingStream> startStreamingRegion(const QRect region, Screencasting::CursorMode mode);
    std::unique_ptr<ScreencastingStream>
    startStreamingVirtualOutput(const QString &name, const QString &description, const QSize &size, Screencasting::CursorMode mode);

    void requestPointerButtonPress(quint32 linuxButton);
    void requestPointerButtonRelease(quint32 linuxButton);
    void requestPointerMotion(const QSizeF &delta);
    void requestPointerMotionAbsolute(ScreencastingStream *const stream, const QPointF &pos);
    void requestPointerAxis(qreal x, qreal y);
    void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);
    void requestKeyboardKeycode(int keycode, bool state);
    void requestKeyboardKeysym(int keysym, bool state);
    void requestTouchDown(quint32 touchPoint, const QPointF &pos);
    void requestTouchMotion(quint32 touchPoint, const QPointF &pos);
    void requestTouchUp(quint32 touchPoint);

    void setParentWindow(QWindow *window, const QString &parentHandle);

private:
    std::unique_ptr<ScreencastingStream> startStreaming(ScreencastingStream *stream);

    uint m_streamInput = 0;
    bool m_waylandAuthenticationRequested = false;

    Screencasting *m_screencasting = nullptr;
};

}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H
