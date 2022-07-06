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
#include <QMap>
#include <QVector>

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
class FakeInput;
class RemoteBuffer;
class Output;
class XdgImporter;
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
    KWayland::Client::XdgImporter *m_xdgImporter = nullptr;

public:
    void authenticate();

    bool isStreamingEnabled() const;
    bool isStreamingAvailable() const;

    void acquireStreamingInput(bool acquire);

    Stream startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode);
    Stream startStreamingWindow(const QMap<int, QVariant> &win, Screencasting::CursorMode mode);
    Stream startStreamingWorkspace(Screencasting::CursorMode mode);
    Stream startStreamingVirtualOutput(const QString &name, const QSize &size, Screencasting::CursorMode mode);
    void stopStreaming(uint32_t nodeid);

    void requestPointerButtonPress(quint32 linuxButton);
    void requestPointerButtonRelease(quint32 linuxButton);
    void requestPointerMotion(const QSizeF &delta);
    void requestPointerMotionAbsolute(const QPointF &pos);
    void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);
    void requestKeyboardKeycode(int keycode, bool state);
    void requestKeyboardKeysym(int keysym, bool state);
    void requestTouchDown(quint32 touchPoint, const QPointF &pos);
    void requestTouchMotion(quint32 touchPoint, const QPointF &pos);
    void requestTouchUp(quint32 touchPoint);

    QMap<quint32, WaylandOutput> screens();

    void setParentWindow(QWindow *window, const QString &parentHandle);

protected Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void removeOutput(quint32 name);

private:
    Stream startStreaming(ScreencastingStream *stream, const QString &iconName, const QString &description, const QVariantMap &streamOptions);
    bool eventFilter(QObject *watched, QEvent *event) override;

    uint m_streamInput = 0;
    bool m_waylandAuthenticationRequested = false;

    quint32 m_output;
    QDateTime m_lastFrameTime;
    QVector<Stream> m_streams;

    QPoint m_streamedScreenPosition;

    QMap<quint32, WaylandOutput> m_outputMap;

    KWayland::Client::FakeInput *m_fakeInput = nullptr;
    Screencasting *m_screencasting = nullptr;
};

}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H
