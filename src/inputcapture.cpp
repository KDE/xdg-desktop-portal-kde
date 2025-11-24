/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 */

#include "inputcapture.h"

#include "dbushelpers.h"
#include "inputcapture_debug.h"
#include "inputcapturebarrier.h"
#include "inputcapturedialog.h"
#include "request.h"
#include "session.h"
#include "utils.h"

#include <KGlobalAccel>
#include <KLocalizedString>
#include <KNotification>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QGuiApplication>
#include <QScreen>

using namespace Qt::StringLiterals;

static QString kwinService()
{
    return u"org.kde.KWin"_s;
}

constexpr int kwinDBusTimeout = 3000;

static QString kwinInputCapturePath()
{
    return u"/org/kde/KWin/EIS/InputCapture"_s;
}

static QString kwinInputCaptureManagerInterface()
{
    return u"org.kde.KWin.EIS.InputCaptureManager"_s;
}

QDBusArgument &operator<<(QDBusArgument &argument, const InputCapturePortal::zone &zone)
{
    argument.beginStructure();
    argument << zone.width << zone.height << zone.x_offset << zone.y_offset;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, InputCapturePortal::zone &zone)
{
    argument.beginStructure();
    argument >> zone.width >> zone.height >> zone.x_offset >> zone.y_offset;
    argument.endStructure();
    return argument;
}

InputCapturePortal::InputCapturePortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<zone>();
    qDBusRegisterMetaType<QList<zone>>();
    qDBusRegisterMetaType<QList<QMap<QString, QVariant>>>();
    qDBusRegisterMetaType<QPair<QPoint, QPoint>>();
    qDBusRegisterMetaType<QList<QPair<QPoint, QPoint>>>();
}

bool InputCapturePortal::setupInputCaptureSession(InputCaptureSession *session, Capabilities capabilities)
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), kwinInputCapturePath(), kwinInputCaptureManagerInterface(), u"addInputCapture"_s);
    msg << capabilities.toInt();
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, kwinDBusTimeout);
    if (!reply.isValid()) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Failed to create KWin input capture:" << reply.error();
        return false;
    }
    session->connect(reply.value());

    connect(session, &Session::closed, session, [session] {
        auto msg = QDBusMessage::createMethodCall(kwinService(), kwinInputCapturePath(), kwinInputCaptureManagerInterface(), u"removeInputCapture"_s);
        msg << session->kwinInputCapture();
        QDBusConnection::sessionBus().call(msg, QDBus::Block, kwinDBusTimeout);
    });
    connect(session, &InputCaptureSession::disabled, this, [this, session] {
        session->state = State::Disabled;
        qCDebug(XdgDesktopPortalKdeInputCapture) << "Disabled session" << session->handle();
        Q_EMIT Disabled(QDBusObjectPath(session->handle()), {});
    });
    connect(session, &InputCaptureSession::deactivated, this, [this, session](uint activationId) {
        session->state = State::Deactivated;
        qCDebug(XdgDesktopPortalKdeInputCapture) << "Deactivated session" << session->handle() << "acitvation_id" << activationId;
        Q_EMIT Deactivated(QDBusObjectPath(session->handle()), {{u"activation_id"_s, activationId}});
    });
    connect(session, &InputCaptureSession::activated, this, [this, session](uint activationId, const QPointF &cursorPosition) {
        session->state = State::Activated;
        qCDebug(XdgDesktopPortalKdeInputCapture) << "Activated session" << session->handle() << "acitvation_id" << activationId << "cursor_position"
                                                 << cursorPosition;

        const auto disableShortcuts = KGlobalAccel::self()->globalShortcut(u"kwin"_s, u"disableInputCapture"_s);
        // Even if the user removed all sequences for this, we use the default action to have an escape hatch
        // Unfortunately KGlobalAccel doesnt have a method to retrieve the default shortcut for another component
        const QKeySequence disableSequence = disableShortcuts.value(0, QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_Escape));

        auto notification = new KNotification(u"inputcapturestarted"_s, KNotification::CloseOnTimeout | KNotification::Persistent, this);
        notification->setTitle(i18nc("@title:notification", "Input Capture started"));
        if (const QString appName = Utils::applicationName(session->appId()); !appName.isEmpty()) {
            notification->setText(xi18nc("@info %1 is the name of the application",
                                         "Input is being managed by %1. Press <shortcut>%2</shortcut> to disable.",
                                         appName,
                                         disableSequence.toString(QKeySequence::NativeText)));
        } else {
            notification->setText(xi18nc("@info",
                                         "Input is being managed by an application. Press <shortcut>%1</shortcut> to disable.",
                                         disableSequence.toString(QKeySequence::NativeText)));
        }
        notification->setIconName(u"dialog-input-devices"_s);
        connect(session, &InputCaptureSession::deactivated, notification, &KNotification::close);
        notification->sendEvent();

        Q_EMIT Activated(QDBusObjectPath(session->handle()), {{u"activation_id"_s, activationId}, {u"cursor_position"_s, cursorPosition}});
    });
    return true;
}

void InputCapturePortal::CreateSession(const QDBusObjectPath &handle,
                                       const QDBusObjectPath &session_handle,
                                       const QString &app_id,
                                       const QString &parent_window,
                                       const QVariantMap &options,
                                       const QDBusMessage &message,
                                       uint &replyResponse,
                                       [[maybe_unused]] QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeInputCapture) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = new InputCaptureSession(this, app_id, session_handle.path());

    if (!session->isValid()) {
        replyResponse = PortalResponse::OtherError;
        return;
    }

    const auto requestedCapabilities = Capabilities::fromInt(options.value(u"capabilities"_s).toUInt());
    if (requestedCapabilities == Capability::None) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "No capabilities requested";
        replyResponse = PortalResponse::OtherError;
        return;
    }

    auto dialog = new InputCaptureDialog(app_id, (requestedCapabilities), this);
    Utils::setParentWindow(dialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequestWithSession(handle, dialog, session);

    delayReply(message, dialog, this, [this, session, requestedCapabilities](DialogResult result) {
        auto response = PortalResponse::fromDialogResult(result);
        QVariantMap results;
        if (result == DialogResult::Accepted) {
            if (!setupInputCaptureSession(session, requestedCapabilities)) {
                response = PortalResponse::OtherError;
            } else {
                results.insert(u"capabilities"_s, static_cast<uint>(requestedCapabilities));
            }
        }
        return QVariantList{response, results};
    });
}

QVariantMap InputCapturePortal::CreateSession2(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeInputCapture) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    [[maybe_unused]] auto *session = new InputCaptureSession(this, app_id, session_handle.path());

    return {};
}

void InputCapturePortal::Start(const QDBusObjectPath &handle,
                               const QDBusObjectPath &session_handle,
                               const QString &app_id,
                               const QString &parent_window,
                               const QVariantMap &options,
                               const QDBusMessage &message,
                               uint &replyResponse,
                               [[maybe_unused]] QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeInputCapture) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to start non-existing session " << session_handle.path();
        replyResponse = PortalResponse::OtherError;
        return;
    }

    const auto requestedCapabilities = Capabilities::fromInt(options.value(u"capabilities"_s).toUInt());
    if (requestedCapabilities == Capability::None) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "No capabilities requested";
        replyResponse = PortalResponse::OtherError;
        return;
    }

    auto dialog = new InputCaptureDialog(app_id, (requestedCapabilities), this);
    Utils::setParentWindow(dialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequestWithSession(handle, dialog, session);

    delayReply(message, dialog, this, [this, session, requestedCapabilities](DialogResult result) {
        auto response = PortalResponse::fromDialogResult(result);
        QVariantMap results;
        if (result == DialogResult::Accepted) {
            if (!setupInputCaptureSession(session, requestedCapabilities)) {
                response = PortalResponse::OtherError;
            } else {
                results.insert(u"capabilities"_s, static_cast<uint>(requestedCapabilities));
            }
        }
        return QVariantList{response, results};
    });
}

uint InputCapturePortal::GetZones(const QDBusObjectPath &handle,
                                  const QDBusObjectPath &session_handle,
                                  const QString &app_id,
                                  const QVariantMap &options,
                                  QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeInputCapture) << "GetZones called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to get zones on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    auto handleZoneChange = [this, session] {
        if (session->state != State::Disabled) {
            if (!QDBusReply(session->disable()).isValid()) {
                qCWarning(XdgDesktopPortalKdeInputCapture()) << "Error disabling capture on zone change";
                session->close();
                return;
            }
        }
        session->clearBarriers();
        qCDebug(XdgDesktopPortalKdeInputCapture) << "Sending  zones changed" << session->handle();
        ++m_zoneId;
        Q_EMIT ZonesChanged(QDBusObjectPath(session->handle()), {{u"zone_set"_s, m_zoneId}});
    };

    results.insert(u"zone_set"_s, m_zoneId);
    QList<zone> zones;

    for (const auto screen : qGuiApp->screens()) {
        const zone zone = {
            .width = static_cast<uint>(screen->geometry().width()),
            .height = static_cast<uint>(screen->geometry().height()),
            .x_offset = screen->geometry().x(),
            .y_offset = screen->geometry().y(),
        };

        // Skip duplicate zones (replicated/mirrored displays)
        if (zones.contains(zone)) {
            qCDebug(XdgDesktopPortalKdeInputCapture) << "Skipping duplicate screen geometry" << screen->geometry()
                                                     << "for screen" << screen->name()
                                                     << "(likely replicated display)";
            continue;
        }

        zones.push_back(zone);
        connect(screen, &QScreen::geometryChanged, session, handleZoneChange);
    }

    connect(qGuiApp, &QGuiApplication::screenAdded, session, handleZoneChange);
    connect(qGuiApp, &QGuiApplication::screenRemoved, session, handleZoneChange);

    results.insert(u"zones"_s, QVariant::fromValue(zones));
    return PortalResponse::Success;
}

uint InputCapturePortal::SetPointerBarriers(const QDBusObjectPath &handle,
                                            const QDBusObjectPath &session_handle,
                                            const QString &app_id,
                                            const QVariantMap &options,
                                            const QList<QVariantMap> &barriers,
                                            uint zone_set,
                                            QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeInputCapture) << "SetPointerBarriers called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    zone_set: " << zone_set;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    barriers: ";

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to set barriers non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    if (zone_set != m_zoneId) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Invalid zone_set " << session_handle.path();
        return PortalResponse::OtherError;
    }

    if (session->state != State::Disabled) {
        if (auto reply = QDBusReply(session->disable()); !reply.isValid()) {
            qCWarning(XdgDesktopPortalKdeInputCapture) << "Error disabling input capture:" << reply.error();
            return PortalResponse::OtherError;
        }
    }
    session->clearBarriers();

    // Build unique screen geometries, skipping replicated displays
    QList<QRect> screenGeometries;
    for (const auto screen : qGuiApp->screens()) {
        const QRect geometry = screen->geometry();
        if (!screenGeometries.contains(geometry)) {
            screenGeometries.append(geometry);
        }
    }

    QList<uint> failedBarriers;

    for (const auto &barrier : barriers) {
        const auto id = barrier.value(u"barrier_id"_s).toUInt();
        int x1;
        int y1;
        int x2;
        int y2;
        const auto position = barrier.value(u"position"_s).value<QDBusArgument>();
        position.beginStructure();
        // (iiii)
        position >> x1 >> y1 >> x2 >> y2;
        position.endStructure();
        qCDebug(XdgDesktopPortalKdeInputCapture) << "        " << id << x1 << y1 << x2 << y2;

        if (id == 0) {
            qCWarning(XdgDesktopPortalKdeInputCapture) << "Invalid barrier id " << id;
            failedBarriers.append(id);
            continue;
        }

        const auto barrierOrFailure = checkAndMakeBarrier(x1, y1, x2, y2, screenGeometries);
        if (auto reason = std::get_if<BarrierFailureReason>(&barrierOrFailure)) {
            switch (*reason) {
            case BarrierFailureReason::Diagonal:
                qCWarning(XdgDesktopPortalKdeInputCapture) << "Disallowed Diagonal barrier " << id;
                break;
            case BarrierFailureReason::NotOnEdge:
                qCWarning(XdgDesktopPortalKdeInputCapture) << "Barrier" << id << "not on any screen edge";
                break;
            case BarrierFailureReason::BetweenScreensOrDoesNotFill:
                qCWarning(XdgDesktopPortalKdeInputCapture) << "Barrier" << id << "doesnt fill or on edge to another screen";
                break;
            }
            failedBarriers.append(id);
        } else {
            session->addBarrier(std::get<1>(barrierOrFailure));
        }
    }
    results.insert(u"failed_barriers"_s, QVariant::fromValue(failedBarriers));
    return PortalResponse::Success;
}

QDBusUnixFileDescriptor
InputCapturePortal::ConnectToEIS(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options, const QDBusMessage &message)
{
    qCDebug(XdgDesktopPortalKdeInputCapture) << "ConnectToEIS called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to call ConnectToEis on non-existing session " << session_handle.path();
        return QDBusUnixFileDescriptor();
    }

    if (session->state != State::Disabled) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to call ConnectToEis on enabled session " << session_handle.path();
        auto error = message.createErrorReply(QDBusError::Failed, u"Session is enabled"_s);
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    QDBusReply<QDBusUnixFileDescriptor> reply = session->connectToEIS();
    if (!reply.isValid()) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Failed to connect to eis" << reply.error();
        auto error = message.createErrorReply(QDBusError::Failed, u"Failed to connect to eis"_s);
        QDBusConnection::sessionBus().send(error);
        return QDBusUnixFileDescriptor();
    }

    return reply.value();
}

uint InputCapturePortal::Enable(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options, QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeInputCapture) << "Enable called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to call Enable on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    if (session->state != State::Disabled) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Session is already enabled" << session_handle.path();
        return PortalResponse::OtherError;
    }

    QDBusReply reply = session->enable();
    if (!reply.isValid()) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Failed to enable session" << reply.error();
        return PortalResponse::OtherError;
    }

    session->state = State::Deactivated;
    return PortalResponse::Success;
}

uint InputCapturePortal::Disable(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options, QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeInputCapture) << "Disable called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to call Enable on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    if (session->state == State::Disabled) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Session is not enabled" << session_handle.path();
        return PortalResponse::OtherError;
    }

    QDBusReply reply = session->enable();
    if (!reply.isValid()) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Failed to disable session" << reply.error();
        return PortalResponse::OtherError;
    }

    return PortalResponse::Success;
}

uint InputCapturePortal::Release(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options, QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeInputCapture) << "Release called with parameters:";
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeInputCapture) << "    options: " << options;

    auto *session = Session::getSession<InputCaptureSession>(session_handle.path());
    if (!session) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Tried to call Enable on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    if (session->state != State::Activated) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Session is not activated" << session_handle.path();
        return PortalResponse::OtherError;
    }

    auto it = options.find(u"cursor_position"_s);
    bool positionSpecified = it != options.end();
    QPointF cursorPosition = positionSpecified ? qdbus_cast<QPointF>(it->value<QDBusArgument>()) : QPointF(); // (dd)

    if (positionSpecified) {
        qCDebug(XdgDesktopPortalKdeInputCapture) << "cursor_position" << cursorPosition;
    } else {
        qCDebug(XdgDesktopPortalKdeInputCapture) << "no cursor position hinted";
    }

    QDBusReply reply = session->release(cursorPosition, positionSpecified);
    if (!reply.isValid()) {
        qCWarning(XdgDesktopPortalKdeInputCapture) << "Failed to release session" << reply.error();
        return PortalResponse::OtherError;
    }

    return PortalResponse::Success;
}

static QString kwinInputCaptureInterface()
{
    return u"org.kde.KWin.EIS.InputCapture"_s;
}

InputCaptureSession::InputCaptureSession(QObject *parent, const QString &appId, const QString &path)
    : Session(parent, appId, path)
    , state(InputCapturePortal::State::Disabled)
{
}

InputCaptureSession::~InputCaptureSession() = default;

void InputCaptureSession::addBarrier(const QPair<QPoint, QPoint> &barrier)
{
    m_barriers.push_back(barrier);
}

void InputCaptureSession::clearBarriers()
{
    m_barriers.clear();
}

QDBusObjectPath InputCaptureSession::kwinInputCapture() const
{
    return m_kwinInputCapture;
}

void InputCaptureSession::connect(const QDBusObjectPath &path)
{
    m_kwinInputCapture = path;
    auto connectSignal = [this](const QString &signalName, const char *slot) {
        QDBusConnection::sessionBus().connect(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), signalName, this, slot);
    };
    connectSignal(u"disabled"_s, SIGNAL(disabled()));
    connectSignal(u"activated"_s, SIGNAL(activated(uint, QPointF)));
    connectSignal(u"deactivated"_s, SIGNAL(deactivated(uint)));
}

QDBusPendingReply<void> InputCaptureSession::enable()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"enable"_s);
    msg << QVariant::fromValue(m_barriers);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<void> InputCaptureSession::disable()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"disable"_s);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<void> InputCaptureSession::release(const QPointF &cusorPosition, bool applyPosition)
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"release"_s);
    msg << cusorPosition << applyPosition;
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

QDBusPendingReply<QDBusUnixFileDescriptor> InputCaptureSession::connectToEIS()
{
    auto msg = QDBusMessage::createMethodCall(kwinService(), m_kwinInputCapture.path(), kwinInputCaptureInterface(), u"connectToEIS"_s);
    return QDBusConnection::sessionBus().asyncCall(msg, kwinDBusTimeout);
}

#include "moc_inputcapture.cpp"
