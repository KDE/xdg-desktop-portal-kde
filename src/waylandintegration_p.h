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

#include <QObject>
#include <QMap>

#include <gbm.h>

#include <epoxy/egl.h>
#include <epoxy/gl.h>

namespace KWayland {
    namespace Client {
        class ConnectionThread;
        class EventQueue;
        class OutputDevice;
        class Registry;
        class RemoteAccessManager;
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

    void initDrm();
    void initEGL();
    void initWayland();

    void bindOutput(int outputName, int outputVersion);
    void startStreaming();
    void stopStreaming();
    QMap<quint32, WaylandOutput> screens();

protected Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void removeOutput(quint32 name);
    void processBuffer(const KWayland::Client::RemoteBuffer *rbuf);
    void setupRegistry();

private:
    bool m_streamingEnabled;
    bool m_registryInitialized;

    QThread *m_thread;

    QMap<quint32, WaylandOutput> m_outputMap;
    QList<KWayland::Client::Output*> m_bindOutputs;

    KWayland::Client::ConnectionThread *m_connection;
    KWayland::Client::EventQueue *m_queue;
    KWayland::Client::Registry *m_registry;
    KWayland::Client::RemoteAccessManager *m_remoteAccessManager;

    qint32 m_drmFd = 0; // for GBM buffer mmap
    gbm_device *m_gbmDevice = nullptr; // for passed GBM buffer retrieval
    struct {
        QList<QByteArray> extensions;
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLContext context = EGL_NO_CONTEXT;
    } m_egl;
};

}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_P_H


