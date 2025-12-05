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

#include "moc_session.cpp"
