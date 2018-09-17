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

#include "screencastcommon.h"
#include "screencaststream.h"

#include <QDBusArgument>
#include <QDBusMetaType>

#include <QEventLoop>
#include <QTimer>

const QDBusArgument &operator >> (const QDBusArgument &arg, ScreenCastCommon::Stream &stream)
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

const QDBusArgument &operator << (QDBusArgument &arg, const ScreenCastCommon::Stream &stream)
{
    arg.beginStructure();
    arg << stream.nodeId;
    arg << stream.map;
    arg.endStructure();

    return arg;
}

Q_DECLARE_METATYPE(ScreenCastCommon::Stream);
Q_DECLARE_METATYPE(ScreenCastCommon::Streams);

ScreenCastCommon::ScreenCastCommon(QObject *parent)
    : QObject(parent)
    , m_streamingEnabled(false)
{
    qDBusRegisterMetaType<ScreenCastCommon::Stream>();
    qDBusRegisterMetaType<ScreenCastCommon::Streams>();
}

ScreenCastCommon::~ScreenCastCommon()
{
}

QVariant ScreenCastCommon::startStreaming(const WaylandIntegration::WaylandOutput &output)
{
    m_stream = new ScreenCastStream(output.resolution());
    m_stream->init();

    connect(WaylandIntegration::waylandIntegration(), &WaylandIntegration::WaylandIntegration::newBuffer, m_stream, &ScreenCastStream::recordFrame);

    connect(m_stream, &ScreenCastStream::startStreaming, this, [this] {
        m_streamingEnabled = true;
        WaylandIntegration::startStreaming();
    });

    connect(m_stream, &ScreenCastStream::stopStreaming, this, &ScreenCastCommon::stopStreaming);

    bool streamReady = false;
    QEventLoop loop;
    connect(m_stream, &ScreenCastStream::streamReady, this, [&loop, &streamReady] {
        loop.quit();
        streamReady = true;
    });

    // HACK wait for stream to be ready
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    disconnect(m_stream, &ScreenCastStream::streamReady, this, nullptr);

    if (!streamReady) {
        return QVariant();
    }

    // TODO support multiple outputs

    WaylandIntegration::bindOutput(output.waylandOutputName(), output.waylandOutputVersion());

    Stream stream;
    stream.nodeId = m_stream->nodeId();
    stream.map = QVariantMap({{QLatin1String("size"), output.resolution()}});
    return QVariant::fromValue<ScreenCastCommon::Streams>({stream});
}

void ScreenCastCommon::stopStreaming()
{
    if (m_streamingEnabled) {
        WaylandIntegration::stopStreaming();
        m_streamingEnabled = false;
        if (m_stream) {
            delete m_stream;
            m_stream = nullptr;
        }
    }
}
