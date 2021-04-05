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

    QThread *m_thread = nullptr;
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

    bool startStreaming(ScreencastingStream *stream, QSharedPointer<KWayland::Client::Output> output);
    bool startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode);
    bool startStreamingWindow(const QByteArray &winid);
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
