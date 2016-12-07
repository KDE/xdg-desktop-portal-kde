/*
 * Copyright © 2016 Red Hat, Inc
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

#include "desktopportal.h"

#include <QDialog>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeDesktopPortal, "xdg-desktop-portal-kde-desktop-portal")

DesktopPortal::DesktopPortal(QObject *parent)
    : QDBusVirtualObject(parent)
    , m_appChooser(new AppChooser())
    , m_fileChooser(new FileChooser())
    , m_notification(new Notification())
{
}

DesktopPortal::~DesktopPortal()
{
    delete m_appChooser;
    delete m_fileChooser;
    delete m_notification;
}

bool DesktopPortal::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    /* Check to make sure we're getting properties on our interface */
    if (message.type() != QDBusMessage::MessageType::MethodCallMessage) {
        return false;
    }

    qCDebug(XdgDesktopPortalKdeDesktopPortal) << message.interface();
    qCDebug(XdgDesktopPortalKdeDesktopPortal) << message.member();
    qCDebug(XdgDesktopPortalKdeDesktopPortal) << message.path();

    QList<QVariant> arguments;
    if (message.interface() == QLatin1String("org.freedesktop.impl.portal.AppChooser")) {
        if (message.member() == QLatin1String("ChooseApplication")) {
            QVariantMap results;
            QVariantMap choices;

            QDBusArgument dbusArgument = message.arguments().at(4).value<QDBusArgument>();
            while (!dbusArgument.atEnd()) {
                dbusArgument >> choices;
            }

            qCDebug(XdgDesktopPortalKdeDesktopPortal) << choices;
            uint response = m_appChooser->ChooseApplication(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                                            message.arguments().at(1).toString(),                       // app_id
                                                            message.arguments().at(2).toString(),                       // parent_window
                                                            message.arguments().at(3).toStringList(),                   // choices
                                                            choices,                                                    // options
                                                            results);
            arguments << response;
            arguments << results;
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.FileChooser")) {
        uint response;
        QVariantMap results;
        QVariantMap choices;

        QDBusArgument dbusArgument = message.arguments().at(4).value<QDBusArgument>();
        while (!dbusArgument.atEnd()) {
            dbusArgument >> choices;
        }

        if (message.member() == QLatin1String("OpenFile")) {
            response = m_fileChooser->OpenFile(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                               message.arguments().at(1).toString(),                       // app_id
                                               message.arguments().at(2).toString(),                       // parent_window
                                               message.arguments().at(3).toString(),                       // title
                                               choices,                                                    // options
                                               results);
        } else if (message.member() == QLatin1String("SaveFile")) {
            response = m_fileChooser->SaveFile(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                               message.arguments().at(1).toString(),                       // app_id
                                               message.arguments().at(2).toString(),                       // parent_window
                                               message.arguments().at(3).toString(),                       // title
                                               choices,                                                    // options
                                               results);
        }

        arguments << response;
        arguments << results;
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Notification")) {
        if (message.member() == QLatin1String("AddNotification")) {
            QVariantMap notificationParams;

            QDBusArgument dbusArgument = message.arguments().at(2).value<QDBusArgument>();
            while (!dbusArgument.atEnd()) {
                dbusArgument >> notificationParams;
            }
            m_notification->AddNotification(message.arguments().at(0).toString(),                           // app_id
                                            message.arguments().at(1).toString(),                           // id
                                            notificationParams);                                            // notification
        } else if (message.member() == QLatin1String("RemoveNotification")) {
            m_notification->RemoveNotification(message.arguments().at(0).toString(),                        // app_id
                                               message.arguments().at(1).toString());                       // id
        }
    }

    QDBusMessage reply = message.createReply();
    reply.setArguments(arguments);
    return connection.send(reply);
}

QString DesktopPortal::introspect(const QString &path) const
{
    QString nodes;

    if (path == QLatin1String("/org/freedesktop/portal/desktop/") || path == QLatin1String("/org/freedesktop/portal/desktop")) {
        nodes = QStringLiteral(
            "<interface name=\"org.freedesktop.impl.portal.AppChooser\">"
            "    <method name=\"ChooseApplication\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
            "        <arg type=\"as\" name=\"choices\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"response\" direction=\"out\"/>"
            "        <arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
            "    </method>"
            "</interface>"
            "<interface name=\"org.freedesktop.impl.portal.FileChooser\">"
            "    <method name=\"OpenFile\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"title\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"response\" direction=\"out\"/>"
            "        <arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
            "    </method>"
            "    <method name=\"SaveFile\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"title\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"response\" direction=\"out\"/>"
            "        <arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
            "    </method>"
            "</interface>"
            "<interface name=\"org.freedesktop.impl.portal.Notification\">"
            "    <method name=\"AddNotification\">"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"id\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"notification\" direction=\"in\"/>"
            "    </method>"
            "    <method name=\"RemoveNotification\">"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"id\" direction=\"in\"/>"
            "    </method>"
            "    <signal name=\"ActionInvoked\">"
            "       <arg type=\"s\" name=\"app_id\"/>"
            "       <arg type=\"s\" name=\"id\"/>"
            "       <arg type=\"s\" name=\"action\"/>"
            "       <arg type=\"av\" name=\"parameter\"/>"
            "    </signal>"
            "</interface>");
    }

    qCDebug(XdgDesktopPortalKdeDesktopPortal) << nodes;

    return nodes;
}
