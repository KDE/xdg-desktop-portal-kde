/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "session.h"
#include "desktopportal.h"
#include "session_debug.h"
#include "sessionadaptor.h"
#include "xdgshortcut.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QLoggingCategory>
#include <screencast_debug.h>

#include <KLocalizedString>
#include <KService>
#include <KStatusNotifierItem>

#include "remotedesktopdialog.h"
#include "utils.h"
#include <QMenu>

static QMap<QString, Session *> sessionList;

using namespace Qt::StringLiterals;

Session::Session(QObject *parent, const QString &appId, const QString &path)
    : QObject(parent)
    , m_appId(appId)
    , m_path(path)
{
    new SessionAdaptor(this);
    if (QDBusConnection::sessionBus().registerObject(path, this)) {
        m_valid = true;
        sessionList.insert(path, this);
    }
}

Session::~Session()
{
}

bool Session::close()
{
    QDBusMessage reply = QDBusMessage::createSignal(m_path, QStringLiteral("org.freedesktop.impl.portal.Session"), QStringLiteral("Closed"));
    const bool result = QDBusConnection::sessionBus().send(reply);

    Q_EMIT closed();

    sessionList.remove(m_path);
    QDBusConnection::sessionBus().unregisterObject(m_path);

    deleteLater();
    m_valid = false;

    return result;
}

Session *Session::getSession(const QString &sessionHandle)
{
    return sessionList.value(sessionHandle);
}

void Session::Close()
{
    close();
}

static QString kwinService()
{
    return u"org.kde.KWin"_s;
}

constexpr int kwinDBusTimeout = 3000;

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

