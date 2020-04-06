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

class ScreenCastStream;

namespace KWayland {
    namespace Client {
        class ConnectionThread;
        class EventQueue;
        class FakeInput;
        class Registry;
        class RemoteAccessManager;
        class RemoteBuffer;
        class Output;
        class PlasmaWindow;
        class PlasmaWindowManagement;
    }
}

namespace WaylandIntegration
{

class WaylandIntegrationPrivate : public WaylandIntegration::WaylandIntegration
{
    Q_OBJECT
public:
    typedef struct {
        uint nodeId;
        QVariantMap map;
    } Stream;
    typedef QList<Stream> Streams;

    WaylandIntegrationPrivate();
    ~WaylandIntegrationPrivate();

    void authenticate();
    void initDrm();
    void initEGL();
    void initWayland();

    bool isEGLInitialized() const;
    bool isStreamingEnabled() const;

    void bindOutput(int outputName, int outputVersion);
    void startStreamingInput();
    bool startStreaming(quint32 outputName);
    void stopStreaming();

    void requestPointerButtonPress(quint32 linuxButton);
    void requestPointerButtonRelease(quint32 linuxButton);
    void requestPointerMotion(const QSizeF &delta);
    void requestPointerMotionAbsolute(const QPointF &pos);
    void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);
    void requestKeyboardKeycode(int keycode, bool state);

    EGLStruct egl();
    KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();
    QMap<quint32, WaylandOutput> screens();
    QVariant streams();

protected Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void removeOutput(quint32 name);
    void processBuffer(const KWayland::Client::RemoteBuffer *rbuf);
    void setupRegistry();

private:
    bool m_eglInitialized = false;
    bool m_streamingEnabled = false;
    bool m_streamInput = false;
    bool m_registryInitialized = false;
    bool m_waylandAuthenticationRequested = false;

    quint32 m_output;
    QDateTime m_lastFrameTime;
    ScreenCastStream *m_stream = nullptr;

    QThread *m_thread = nullptr;

    QPoint m_streamedScreenPosition;

    QMap<quint32, WaylandOutput> m_outputMap;
    QList<KWayland::Client::Output*> m_bindOutputs;

    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::EventQueue *m_queue = nullptr;
    KWayland::Client::FakeInput *m_fakeInput = nullptr;
    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::RemoteAccessManager *m_remoteAccessManager = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;

    qint32 m_drmFd = 0; // for GBM buffer mmap
    gbm_device *m_gbmDevice = nullptr; // for passed GBM buffer retrieval

    EGLStruct m_egl;
};

}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H


