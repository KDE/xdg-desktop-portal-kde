/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "waylandintegration.h"
#include "screencast.h"
#include "screencasting.h"
#include "waylandintegration_p.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QGuiApplication>

#include <KNotification>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>
#include <QTimer>

#include <QImage>

#include <KLocalizedString>

// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <KWayland/Client/registry.h>

// system
#include <fcntl.h>
#include <unistd.h>

#include <KStatusNotifierItem>
#include <KWayland/Client/fakeinput.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeWaylandIntegration, "xdp-kde-wayland-integration")

Q_GLOBAL_STATIC(WaylandIntegration::WaylandIntegrationPrivate, globalWaylandIntegration)

static QDebug operator<<(QDebug dbg, const WaylandIntegration::WaylandIntegrationPrivate::Stream &c)
{
    dbg.nospace() << "Stream(" << c.map << ", " << c.nodeId << ")";
    return dbg.space();
}

void WaylandIntegration::init()
{
    globalWaylandIntegration->initWayland();
}

void WaylandIntegration::authenticate()
{
    globalWaylandIntegration->authenticate();
}

bool WaylandIntegration::isStreamingEnabled()
{
    return globalWaylandIntegration->isStreamingEnabled();
}

bool WaylandIntegration::isStreamingAvailable()
{
    return globalWaylandIntegration->isStreamingAvailable();
}

void WaylandIntegration::startStreamingInput()
{
    globalWaylandIntegration->startStreamingInput();
}

bool WaylandIntegration::startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingOutput(outputName, mode);
}

bool WaylandIntegration::startStreamingWindow(const QMap<int, QVariant> &win)
{
    return globalWaylandIntegration->startStreamingWindow(win);
}

void WaylandIntegration::stopAllStreaming()
{
    globalWaylandIntegration->stopAllStreaming();
}

void WaylandIntegration::requestPointerButtonPress(quint32 linuxButton)
{
    globalWaylandIntegration->requestPointerButtonPress(linuxButton);
}

void WaylandIntegration::requestPointerButtonRelease(quint32 linuxButton)
{
    globalWaylandIntegration->requestPointerButtonRelease(linuxButton);
}

void WaylandIntegration::requestPointerMotion(const QSizeF &delta)
{
    globalWaylandIntegration->requestPointerMotion(delta);
}

void WaylandIntegration::requestPointerMotionAbsolute(const QPointF &pos)
{
    globalWaylandIntegration->requestPointerMotionAbsolute(pos);
}

void WaylandIntegration::requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta)
{
    globalWaylandIntegration->requestPointerAxisDiscrete(axis, delta);
}

void WaylandIntegration::requestKeyboardKeycode(int keycode, bool state)
{
    globalWaylandIntegration->requestKeyboardKeycode(keycode, state);
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::screens()
{
    return globalWaylandIntegration->screens();
}

QVariant WaylandIntegration::streams()
{
    return globalWaylandIntegration->streams();
}

// Thank you kscreen
void WaylandIntegration::WaylandOutput::setOutputType(const QString &type)
{
    const auto embedded = {
        QLatin1String("LVDS"),
        QLatin1String("IDP"),
        QLatin1String("EDP"),
        QLatin1String("LCD"),
    };

    for (const QLatin1String &pre : embedded) {
        if (type.startsWith(pre, Qt::CaseInsensitive)) {
            m_outputType = OutputType::Laptop;
            return;
        }
    }

    if (type.contains(QLatin1String("VGA")) || type.contains(QLatin1String("DVI")) || type.contains(QLatin1String("HDMI"))
        || type.contains(QLatin1String("Panel")) || type.contains(QLatin1String("DisplayPort")) || type.startsWith(QLatin1String("DP"))
        || type.contains(QLatin1String("unknown"))) {
        m_outputType = OutputType::Monitor;
    } else if (type.contains(QLatin1String("TV"))) {
        m_outputType = OutputType::Television;
    } else {
        m_outputType = OutputType::Monitor;
    }
}

const QDBusArgument &operator>>(const QDBusArgument &arg, WaylandIntegration::WaylandIntegrationPrivate::Stream &stream)
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

const QDBusArgument &operator<<(QDBusArgument &arg, const WaylandIntegration::WaylandIntegrationPrivate::Stream &stream)
{
    arg.beginStructure();
    arg << stream.nodeId;
    arg << stream.map;
    arg.endStructure();

    return arg;
}

Q_DECLARE_METATYPE(WaylandIntegration::WaylandIntegrationPrivate::Stream)
Q_DECLARE_METATYPE(WaylandIntegration::WaylandIntegrationPrivate::Streams)

KWayland::Client::PlasmaWindowManagement *WaylandIntegration::plasmaWindowManagement()
{
    return globalWaylandIntegration->plasmaWindowManagement();
}

WaylandIntegration::WaylandIntegration *WaylandIntegration::waylandIntegration()
{
    return globalWaylandIntegration;
}

WaylandIntegration::WaylandIntegrationPrivate::WaylandIntegrationPrivate()
    : WaylandIntegration()
    , m_registryInitialized(false)
    , m_registry(nullptr)
    , m_fakeInput(nullptr)
    , m_screencasting(nullptr)
{
    qDBusRegisterMetaType<WaylandIntegrationPrivate::Stream>();
    qDBusRegisterMetaType<WaylandIntegrationPrivate::Streams>();
}

WaylandIntegration::WaylandIntegrationPrivate::~WaylandIntegrationPrivate() = default;

bool WaylandIntegration::WaylandIntegrationPrivate::isStreamingEnabled() const
{
    return !m_streams.isEmpty();
}

bool WaylandIntegration::WaylandIntegrationPrivate::isStreamingAvailable() const
{
    return m_screencasting;
}

void WaylandIntegration::WaylandIntegrationPrivate::startStreamingInput()
{
    m_streamInput = true;
}

bool WaylandIntegration::WaylandIntegrationPrivate::startStreamingWindow(const QMap<int, QVariant> &win)
{
    auto uuid = win[KWayland::Client::PlasmaWindowModel::Uuid].toString();
    return startStreaming(m_screencasting->createWindowStream(uuid, Screencasting::Hidden), {}, win);
}

bool WaylandIntegration::WaylandIntegrationPrivate::startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode)
{
    auto output = m_outputMap.value(outputName).output();

    return startStreaming(m_screencasting->createOutputStream(output.data(), mode), output, {});
}

bool WaylandIntegration::WaylandIntegrationPrivate::startStreaming(ScreencastingStream *stream,
                                                                   QSharedPointer<KWayland::Client::Output> output,
                                                                   const QMap<int, QVariant> &win)
{
    QEventLoop loop;
    bool streamReady = false;
    connect(stream, &ScreencastingStream::failed, this, [&](const QString &error) {
        qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "failed to start streaming" << stream << error;

        KNotification *notification = new KNotification(QStringLiteral("screencastfailure"), KNotification::CloseOnTimeout);
        notification->setTitle(i18n("Failed to start screencasting"));
        notification->setText(error);
        notification->setIconName(QStringLiteral("dialog-error"));
        notification->sendEvent();

        streamReady = false;
        loop.quit();
    });
    connect(stream, &ScreencastingStream::created, this, [&](uint32_t nodeid) {
        Stream s;
        s.stream = stream;
        s.nodeId = nodeid;
        if (output) {
            m_streamedScreenPosition = output->globalPosition();
            s.map = {
                {QLatin1String("size"), output->pixelSize()},
                {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Monitor)},
            };
        } else {
            s.map = {{QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Window)}};
        }
        m_streams.append(s);
        startStreamingInput();

        connect(stream, &ScreencastingStream::closed, this, [this, nodeid] {
            stopStreaming(nodeid);
        });
        streamReady = true;

        auto item = new KStatusNotifierItem(stream);
        item->setStandardActionsEnabled(false);
        if (output) {
            item->setTitle(i18n("Recording screen \"%1\"...", output->model()));
            item->setIconByName("video-display");
        } else {
            auto name = win[Qt::DecorationRole].value<QIcon>().name();
            item->setIconByName(name.isEmpty() ? "applications-all" : name);
            item->setTitle(i18n("Recording window \"%1\"...", win[Qt::DisplayRole].toString()));
        }
        item->setOverlayIconByName("media-record");
        item->setToolTip(item->iconName(), item->title(), i18n("Press to cancel"));
        item->setStatus(KStatusNotifierItem::Active);
        connect(item, &KStatusNotifierItem::activateRequested, stream, [=] {
            stopStreaming(nodeid);
            stream->deleteLater();
        });

        loop.quit();
    });
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    return streamReady;
}

void WaylandIntegration::WaylandIntegrationPrivate::Stream::close()
{
    stream->deleteLater();
}

void WaylandIntegration::WaylandIntegrationPrivate::stopAllStreaming()
{
    for (auto &stream : m_streams) {
        stream.close();
    }
    m_streams.clear();

    m_streamInput = false;
    // First unbound outputs and destroy remote access manager so we no longer receive buffers

    Q_EMIT streamingStopped();
}

void WaylandIntegration::WaylandIntegrationPrivate::stopStreaming(uint32_t nodeid)
{
    for (auto it = m_streams.begin(), itEnd = m_streams.end(); it != itEnd; ++it) {
        if (it->nodeId == nodeid) {
            it->close();
            m_streams.erase(it);
            break;
        }
    }

    if (m_streams.isEmpty()) {
        stopAllStreaming();
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestPointerButtonPress(quint32 linuxButton)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestPointerButtonPress(linuxButton);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestPointerButtonRelease(quint32 linuxButton)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestPointerButtonRelease(linuxButton);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestPointerMotion(const QSizeF &delta)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestPointerMove(delta);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestPointerMotionAbsolute(const QPointF &pos)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestPointerMoveAbsolute(pos + m_streamedScreenPosition);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestPointerAxis(axis, delta);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestKeyboardKeycode(int keycode, bool state)
{
    if (m_streamInput && m_fakeInput) {
        if (state) {
            m_fakeInput->requestKeyboardKeyPress(keycode);
        } else {
            m_fakeInput->requestKeyboardKeyRelease(keycode);
        }
    }
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::WaylandIntegrationPrivate::screens()
{
    return m_outputMap;
}

QVariant WaylandIntegration::WaylandIntegrationPrivate::streams()
{
    return QVariant::fromValue<WaylandIntegrationPrivate::Streams>(m_streams);
}

void WaylandIntegration::WaylandIntegrationPrivate::authenticate()
{
    if (!m_waylandAuthenticationRequested) {
        m_fakeInput->authenticate(i18n("xdg-desktop-portals-kde"), i18n("Remote desktop"));
        m_waylandAuthenticationRequested = true;
    }
}

KWayland::Client::PlasmaWindowManagement *WaylandIntegration::WaylandIntegrationPrivate::plasmaWindowManagement()
{
    return m_windowManagement;
}

void WaylandIntegration::WaylandIntegrationPrivate::addOutput(quint32 name, quint32 version)
{
    QSharedPointer<KWayland::Client::Output> output(new KWayland::Client::Output(this));
    output->setup(m_registry->bindOutput(name, version));

    connect(output.data(), &KWayland::Client::Output::changed, this, [this, name, version, output]() {
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Adding output:";
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    manufacturer: " << output->manufacturer();
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    model: " << output->model();
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    resolution: " << output->pixelSize();

        WaylandOutput portalOutput;
        portalOutput.setOutput(output);
        portalOutput.setWaylandOutputName(name);
        portalOutput.setWaylandOutputVersion(version);

        m_outputMap.insert(name, portalOutput);
    });
}

void WaylandIntegration::WaylandIntegrationPrivate::removeOutput(quint32 name)
{
    WaylandOutput output = m_outputMap.take(name);
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Removing output:";
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    manufacturer: " << output.manufacturer();
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    model: " << output.model();
}

void WaylandIntegration::WaylandIntegrationPrivate::initWayland()
{
    auto connection = KWayland::Client::ConnectionThread::fromApplication(QGuiApplication::instance());

    if (!connection) {
        return;
    }

    m_registry = new KWayland::Client::Registry(this);

    connect(m_registry, &KWayland::Client::Registry::fakeInputAnnounced, this, [this](quint32 name, quint32 version) {
        m_fakeInput = m_registry->createFakeInput(name, version, this);
    });
    connect(m_registry, &KWayland::Client::Registry::outputAnnounced, this, &WaylandIntegrationPrivate::addOutput);
    connect(m_registry, &KWayland::Client::Registry::outputRemoved, this, &WaylandIntegrationPrivate::removeOutput);

    connect(m_registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this](const QByteArray &interfaceName, quint32 name, quint32 version) {
        if (interfaceName != "zkde_screencast_unstable_v1")
            return;
        m_screencasting = new Screencasting(m_registry, name, version, this);
    });
    connect(m_registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, this, [this](quint32 name, quint32 version) {
        m_windowManagement = m_registry->createPlasmaWindowManagement(name, version, this);
        Q_EMIT waylandIntegration()->plasmaWindowManagementInitialized();
    });

    connect(m_registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Registry initialized";
    });

    m_registry->create(connection);
    m_registry->setup();
}
