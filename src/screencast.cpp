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

#include "screencast.h"
#include "session.h"
#include "screencaststream.h"
#include "screenchooserdialog.h"
#include "waylandintegration.h"

#include <QDBusMetaType>
#include <QDBusError>
#include <QDBusArgument>
#include <QDBusConnection>

#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenCast, "xdp-kde-screencast")

Q_DECLARE_METATYPE(ScreenCastPortal::Stream);
Q_DECLARE_METATYPE(ScreenCastPortal::Streams);

const QDBusArgument &operator >> (const QDBusArgument &arg, ScreenCastPortal::Stream &stream)
{
    arg.beginStructure();
    arg >> stream.nodeId;

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant map;
        arg.beginMapEntry();
        arg >> key >> map;
        arg.endMapEntry();
        stream.map.insert(key, map);
    }
    arg.endMap();
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator << (QDBusArgument &arg, const ScreenCastPortal::Stream &stream)
{
    arg.beginStructure();
    arg << stream.nodeId;
    arg << stream.map;
    arg.endStructure();

    return arg;
}

ScreenCastPortal::ScreenCastPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_streamingEnabled(false)
{
    qDBusRegisterMetaType<ScreenCastPortal::Stream>();
    qDBusRegisterMetaType<ScreenCastPortal::Streams>();
}

ScreenCastPortal::~ScreenCastPortal()
{
    m_stream->deleteLater();
}

void ScreenCastPortal::createPipeWireStream(const QSize &resolution)
{
    m_stream = new ScreenCastStream(resolution);
    m_stream->init();

    connect(m_stream, &ScreenCastStream::streamReady, this, [] (uint nodeId) {
        qCDebug(XdgDesktopPortalKdeScreenCast) << "Pipewire stream is ready: " << nodeId;
    });

    connect(WaylandIntegration::waylandIntegration(), &WaylandIntegration::WaylandIntegration::newBuffer, m_stream, &ScreenCastStream::recordFrame);

    connect(m_stream, &ScreenCastStream::startStreaming, this, [this] {
        qCDebug(XdgDesktopPortalKdeScreenCast) << "Start streaming";
        m_streamingEnabled = true;
        WaylandIntegration::startStreaming();
    });

    connect(m_stream, &ScreenCastStream::stopStreaming, this, &ScreenCastPortal::stopStreaming);
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

    connect(session, &Session::closed, [this] () {
        stopStreaming();
    });

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

    uint types = Monitor;

    ScreenCastSession *session = qobject_cast<ScreenCastSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to select sources on non-existing session " << session_handle.path();
        return 2;
    }

    if (options.contains(QLatin1String("multiple"))) {
        session->setMultipleSources(options.value(QLatin1String("multiple")).toBool());
    }

    if (options.contains(QLatin1String("types"))) {
        types = (SourceType)(options.value(QLatin1String("types")).toUInt());
    }

    if (types == Window) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Screen cast of a window is not implemented";
        return 2;
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

    ScreenCastSession *session = qobject_cast<ScreenCastSession*>(Session::getSession(session_handle.path()));

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to select sources on non-existing session " << session_handle.path();
        return 2;
    }

    // TODO check whether we got some outputs?
    if (WaylandIntegration::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Failed to show dialog as there is no screen to select";
        return 2;
    }

    QScopedPointer<ScreenChooserDialog, QScopedPointerDeleteLater> screenDialog(new ScreenChooserDialog(session->multipleSources()));

    if (screenDialog->exec()) {
        WaylandIntegration::WaylandOutput selectedOutput = WaylandIntegration::screens().value(screenDialog->selectedScreens().first());

        // Initialize PipeWire
        createPipeWireStream(selectedOutput.resolution());

        // HACK wait for stream to be ready
        bool streamReady = false;
        QEventLoop loop;
        connect(m_stream, &ScreenCastStream::streamReady, this, [&loop, &streamReady] {
            loop.quit();
            streamReady = true;
        });

        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();

        disconnect(m_stream, &ScreenCastStream::streamReady, this, nullptr);

        if (!streamReady) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "Pipewire stream is not ready to be streamed";
            return 2;
        }

        // TODO support multiple outputs

        qCDebug(XdgDesktopPortalKdeScreenCast) << "Pipewire node id: " << m_stream->nodeId();

        WaylandIntegration::bindOutput(selectedOutput.waylandOutputName(), selectedOutput.waylandOutputVersion());

        Stream stream;
        stream.nodeId = m_stream->nodeId();
        stream.map = QVariantMap({{QLatin1String("size"), selectedOutput.resolution()}});
        results.insert(QLatin1String("streams"), QVariant::fromValue<ScreenCastPortal::Streams>({stream}));

        return 0;
    }

    return 0;
}

void ScreenCastPortal::stopStreaming()
{
    if (m_streamingEnabled) {
        qCDebug(XdgDesktopPortalKdeScreenCast) << "Stop streaming";
        WaylandIntegration::stopStreaming();
        m_streamingEnabled = false;
        delete m_stream;
        m_stream = nullptr;
    }
}
