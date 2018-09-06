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

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENCAST_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENCAST_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QSize>

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

class ScreenChooserDialog;
class ScreenCastStream;

class ScreenCastPortalOutput
{
    enum OutputType {
        Laptop,
        Monitor,
        Television
    };

    void setOutputType(const QString &type);

    QString manufacturer;
    QString model;
    QSize resolution;
    OutputType outputType;

    // Needed for later output binding
    int waylandOutputName;
    int waylandOutputVersion;

    friend class ScreenCastPortal;
    friend class ScreenChooserDialog;
};

class ScreenCastPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.ScreenCast")
    Q_PROPERTY(uint version READ version)
    Q_PROPERTY(uint AvailableSourceTypes READ AvailableSourceTypes)

public:
    typedef struct {
        uint nodeId;
        QVariantMap map;
    } Stream;
    typedef QList<Stream> Streams;

    enum SourceType {
        Any = 0,
        Monitor,
        Window
    };

    explicit ScreenCastPortal(QObject *parent);
    ~ScreenCastPortal();

    uint version() const { return 1; }
    uint AvailableSourceTypes() const { return Monitor; };

public Q_SLOTS:
    uint CreateSession(const QDBusObjectPath &handle,
                       const QDBusObjectPath &session_handle,
                       const QString &app_id,
                       const QVariantMap &options,
                       QVariantMap &results);

    uint SelectSources(const QDBusObjectPath &handle,
                       const QDBusObjectPath &session_handle,
                       const QString &app_id,
                       const QVariantMap &options,
                       QVariantMap &results);

    uint Start(const QDBusObjectPath &handle,
               const QDBusObjectPath &session_handle,
               const QString &app_id,
               const QString &parent_window,
               const QVariantMap &options,
               QVariantMap &results);

private Q_SLOTS:
    void addOutput(quint32 name, quint32 version);
    void removeOutput(quint32 name);
    void processBuffer(const KWayland::Client::RemoteBuffer *rbuf);
    void setupRegistry();
    void stopStreaming();

private:
    void createPipeWireStream(const QSize &resolution);
    void initDrm();
    void initEGL();
    void initWayland();

    bool m_registryInitialized;
    bool m_streamingEnabled;

    QMap<quint32, ScreenCastPortalOutput> m_outputMap;
    QList<KWayland::Client::Output*> m_bindOutputs;

    QThread *m_thread;

    ScreenCastStream *m_stream;

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

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENCAST_H

