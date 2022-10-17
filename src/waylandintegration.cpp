/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "waylandintegration.h"
#include "screencast.h"
#include "screencasting.h"
#include "waylandintegration_debug.h"
#include "waylandintegration_p.h"

#include <QDBusMetaType>
#include <QGuiApplication>

#include <KNotification>
#include <QEventLoop>
#include <QImage>
#include <QMenu>
#include <QScreen>
#include <QThread>
#include <QTimer>

#include <KLocalizedString>

// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/xdgforeign.h>

// system
#include <fcntl.h>
#include <unistd.h>

#include <KStatusNotifierItem>
#include <KWayland/Client/fakeinput.h>

Q_GLOBAL_STATIC(WaylandIntegration::WaylandIntegrationPrivate, globalWaylandIntegration)

namespace WaylandIntegration
{
QDebug operator<<(QDebug dbg, const Stream &c)
{
    dbg.nospace() << "Stream(" << c.map << ", " << c.nodeId << ")";
    return dbg.space();
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Stream &stream)
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

const QDBusArgument &operator<<(QDBusArgument &arg, const Stream &stream)
{
    arg.beginStructure();
    arg << stream.nodeId;
    arg << stream.map;
    arg.endStructure();

    return arg;
}

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

void WaylandIntegration::acquireStreamingInput(bool acquire)
{
    globalWaylandIntegration->acquireStreamingInput(acquire);
}

WaylandIntegration::Stream WaylandIntegration::startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingOutput(outputName, mode);
}

WaylandIntegration::Stream WaylandIntegration::startStreamingWorkspace(Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingWorkspace(mode);
}

WaylandIntegration::Stream WaylandIntegration::startStreamingVirtual(const QString &name, const QSize &size, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingVirtualOutput(name, size, mode);
}

WaylandIntegration::Stream WaylandIntegration::startStreamingWindow(const QMap<int, QVariant> &win, Screencasting::CursorMode mode)
{
    return globalWaylandIntegration->startStreamingWindow(win, mode);
}

void WaylandIntegration::stopStreaming(uint node)
{
    globalWaylandIntegration->stopStreaming(node);
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

void WaylandIntegration::requestTouchDown(quint32 touchPoint, const QPointF &pos)
{
    globalWaylandIntegration->requestTouchDown(touchPoint, pos);
}

void WaylandIntegration::requestTouchMotion(quint32 touchPoint, const QPointF &pos)
{
    globalWaylandIntegration->requestTouchMotion(touchPoint, pos);
}

void WaylandIntegration::requestTouchUp(quint32 touchPoint)
{
    globalWaylandIntegration->requestTouchUp(touchPoint);
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::screens()
{
    return globalWaylandIntegration->screens();
}

void WaylandIntegration::setParentWindow(QWindow *window, const QString &parentWindow)
{
    globalWaylandIntegration->setParentWindow(window, parentWindow);
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
    qDBusRegisterMetaType<Stream>();
    qDBusRegisterMetaType<Streams>();
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

void WaylandIntegration::WaylandIntegrationPrivate::acquireStreamingInput(bool acquire)
{
    if (acquire) {
        ++m_streamInput;
    } else {
        Q_ASSERT(m_streamInput > 0);
        --m_streamInput;
    }
}

WaylandIntegration::Stream WaylandIntegration::WaylandIntegrationPrivate::startStreamingWindow(const QMap<int, QVariant> &win,
                                                                                               Screencasting::CursorMode cursorMode)
{
    auto uuid = win[KWayland::Client::PlasmaWindowModel::Uuid].toString();
    QString iconName = win[Qt::DecorationRole].value<QIcon>().name();
    iconName = iconName.isEmpty() ? QStringLiteral("applications-all") : iconName;
    return startStreaming(m_screencasting->createWindowStream(uuid, cursorMode),
                          iconName,
                          i18n("Recording window \"%1\"...", win[Qt::DisplayRole].toString()),
                          {{QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Window)}});
}

WaylandIntegration::Stream WaylandIntegration::WaylandIntegrationPrivate::startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode)
{
    auto output = m_outputMap.value(outputName).output();
    m_streamedScreenPosition = output->globalPosition();
    return startStreaming(m_screencasting->createOutputStream(output.data(), mode),
                          QStringLiteral("media-record"),
                          i18n("Recording screen \"%1\"...", output->model()),
                          {
                              {QLatin1String("size"), output->pixelSize()},
                              {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Monitor)},
                          });
}

WaylandIntegration::Stream WaylandIntegration::WaylandIntegrationPrivate::startStreamingWorkspace(Screencasting::CursorMode mode)
{
    QRect workspace;
    const auto screens = qGuiApp->screens();
    for (QScreen *screen : screens) {
        workspace |= screen->geometry();
    }
    m_streamedScreenPosition = workspace.topLeft();
    return startStreaming(m_screencasting->createRegionStream(workspace, 1, mode),
                          QStringLiteral("media-record"),
                          i18n("Recording workspace..."),
                          {
                              {QLatin1String("size"), workspace.size()},
                              {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Monitor)},
                          });
}

WaylandIntegration::Stream
WaylandIntegration::WaylandIntegrationPrivate::startStreamingVirtualOutput(const QString &name, const QSize &size, Screencasting::CursorMode mode)
{
    return startStreaming(m_screencasting->createVirtualOutputStream(name, size, 1, mode),
                          QStringLiteral("media-record"),
                          i18n("Recording virtual output '%1'...", name),
                          {
                              {QLatin1String("size"), size},
                              {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Virtual)},
                          });
}

WaylandIntegration::Stream WaylandIntegration::WaylandIntegrationPrivate::startStreaming(ScreencastingStream *stream,
                                                                                         const QString &iconName,
                                                                                         const QString &description,
                                                                                         const QVariantMap &streamOptions)
{
    QEventLoop loop;
    Stream ret;

    connect(stream, &ScreencastingStream::failed, this, [&](const QString &error) {
        qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "failed to start streaming" << stream << error;

        KNotification *notification = new KNotification(QStringLiteral("screencastfailure"), KNotification::CloseOnTimeout);
        notification->setTitle(i18n("Failed to start screencasting"));
        notification->setText(error);
        notification->setIconName(QStringLiteral("dialog-error"));
        notification->sendEvent();

        loop.quit();
    });
    connect(stream, &ScreencastingStream::created, this, [&](uint32_t nodeid) {
        ret.stream = stream;
        ret.nodeId = nodeid;
        ret.map = streamOptions;
        m_streams.append(ret);

        connect(stream, &ScreencastingStream::closed, this, [this, nodeid] {
            stopStreaming(nodeid);
        });
        Q_ASSERT(ret.isValid());
        qDebug() << "start streaming" << ret << m_streamedScreenPosition;

        auto item = new KStatusNotifierItem(stream);
        item->setStandardActionsEnabled(false);
        item->setTitle(description);
        item->setIconByName(iconName);
        item->setOverlayIconByName(QStringLiteral("media-record"));
        item->setStatus(KStatusNotifierItem::Active);
        connect(item, &KStatusNotifierItem::activateRequested, stream, [item] {
            item->contextMenu()->show();
        });
        auto menu = new QMenu;
        auto stopAction =
            new QAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18nc("@action:inmenu stops screen/window recording", "Stop Recording"));
        connect(stopAction, &QAction::triggered, this, [this, nodeid, stream] {
            stopStreaming(nodeid);
            stream->deleteLater();
        });
        menu->addAction(stopAction);
        item->setContextMenu(menu);

        loop.quit();
    });
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();
    return ret;
}

void WaylandIntegration::Stream::close()
{
    stream->deleteLater();
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

void WaylandIntegration::WaylandIntegrationPrivate::requestTouchDown(quint32 touchPoint, const QPointF &pos)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestTouchDown(touchPoint, pos);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestTouchMotion(quint32 touchPoint, const QPointF &pos)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestTouchMotion(touchPoint, pos);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::requestTouchUp(quint32 touchPoint)
{
    if (m_streamInput && m_fakeInput) {
        m_fakeInput->requestTouchUp(touchPoint);
    }
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::WaylandIntegrationPrivate::screens()
{
    return m_outputMap;
}

static const char *windowParentHandlePropertyName = "waylandintegration-parentHandle";
void WaylandIntegration::WaylandIntegrationPrivate::setParentWindow(QWindow *window, const QString &parentHandle)
{
    if (!m_xdgImporter) {
        return;
    }

    if (window->isVisible()) {
        auto importedParent = m_xdgImporter->importTopLevel(parentHandle, window);
        auto surface = KWayland::Client::Surface::fromWindow(window);
        importedParent->setParentOf(surface);
    }

    window->setProperty(windowParentHandlePropertyName, parentHandle);
    window->installEventFilter(this);
}

bool WaylandIntegration::WaylandIntegrationPrivate::eventFilter(QObject *watched, QEvent *event)
{
    const bool ret = WaylandIntegration::WaylandIntegration::eventFilter(watched, event);
    QWindow *window = static_cast<QWindow *>(watched);
    if (event->type() == QEvent::Expose && window->isExposed()) {
        const QString parentHandle = window->property(windowParentHandlePropertyName).toString();
        auto importedParent = m_xdgImporter->importTopLevel(parentHandle, window);
        importedParent->setParentOf(KWayland::Client::Surface::fromWindow(window));
    }
    return ret;
}

void WaylandIntegration::WaylandIntegrationPrivate::authenticate()
{
    if (!m_waylandAuthenticationRequested) {
        m_fakeInput->authenticate(QStringLiteral("xdg-desktop-portals-kde"), i18n("Remote desktop"));
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
    connect(m_registry, &KWayland::Client::Registry::importerUnstableV2Announced, this, [this](quint32 name, quint32 version) {
        m_xdgImporter = m_registry->createXdgImporter(name, std::min(version, quint32(1)), this);
    });
    connect(m_registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Registry initialized";
    });

    m_registry->create(connection);
    m_registry->setup();
}
