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
}
}

namespace WaylandIntegration
{
class WaylandIntegrationPrivate : public WaylandIntegration::WaylandIntegration
{
    Q_OBJECT
public:
    WaylandIntegrationPrivate();
    ~WaylandIntegrationPrivate();

    void initWayland();

    KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

protected Q_SLOTS:
    void setupRegistry();

private:
    bool m_registryInitialized = false;

    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::EventQueue *m_queue = nullptr;
    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;

public:
    struct Stream {
        ScreencastingStream *stream = nullptr;
        uint nodeId;
        QVariantMap map;

        void close();
    };
    typedef QVector<Stream> Streams;

    void authenticate();

    bool isStreamingEnabled() const;
    bool isStreamingAvailable() const;

    void startStreamingInput();

    bool startStreaming(ScreencastingStream *stream, QSharedPointer<KWayland::Client::Output> output, const QMap<int, QVariant> &win);
    bool startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode);
    bool startStreamingWindow(const QMap<int, QVariant> &win);
    void stopStreaming(uint32_t nodeid);
    void stopAllStreaming();

    void requestPointerButtonPress(quint32 linuxButton);
    void requestPointerButtonRelease(quint32 linuxButton);
    void requestPointerMotion(const QSizeF &delta);
    void requestPointerMotionAbsolute(const QPointF &pos);
    void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);
    void requestKeyboardKeycode(int keycode, bool state);
    void bindOutput(int outputName, int outputVersion);

    QMap<quint32, WaylandOutput> screens();
    QVariant streams();

protected Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void removeOutput(quint32 name);

private:
    bool m_streamInput = false;
    bool m_waylandAuthenticationRequested = false;

    quint32 m_output;
    QDateTime m_lastFrameTime;
    QVector<Stream> m_streams;

    QPoint m_streamedScreenPosition;

    QMap<quint32, WaylandOutput> m_outputMap;
    QList<KWayland::Client::Output *> m_bindOutputs;

    KWayland::Client::FakeInput *m_fakeInput = nullptr;
    Screencasting *m_screencasting = nullptr;
};

}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H
