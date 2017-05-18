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
    , m_email(new Email())
    , m_fileChooser(new FileChooser())
    , m_inhibit(new Inhibit())
    , m_notification(new Notification())
    , m_print(new Print())
{
}

DesktopPortal::~DesktopPortal()
{
    delete m_appChooser;
    delete m_email;
    delete m_fileChooser;
    delete m_inhibit;
    delete m_notification;
    delete m_print;
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
            dbusArgument >> choices;

            uint response = m_appChooser->chooseApplication(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                                            message.arguments().at(1).toString(),                       // app_id
                                                            message.arguments().at(2).toString(),                       // parent_window
                                                            message.arguments().at(3).toStringList(),                   // choices
                                                            choices,                                                    // options
                                                            results);
            arguments << response;
            arguments << results;
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Email")) {
        uint response = 2;
        QVariantMap results;
        QVariantMap options;

        QDBusArgument dbusArgument = message.arguments().at(3).value<QDBusArgument>();
        dbusArgument >> options;

        if (message.member() == QLatin1String("ComposeEmail")) {
            response = m_email->composeEmail(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                             message.arguments().at(1).toString(),                       // app_id
                                             message.arguments().at(2).toString(),                       // parent_window
                                             options,                                                    // options
                                             results);
        }

        arguments << response;
        arguments << results;
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.FileChooser")) {
        uint response = 2;
        QVariantMap results;
        QVariantMap choices;

        QDBusArgument dbusArgument = message.arguments().at(4).value<QDBusArgument>();
        dbusArgument >> choices;

        if (message.member() == QLatin1String("OpenFile")) {
            response = m_fileChooser->openFile(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                               message.arguments().at(1).toString(),                       // app_id
                                               message.arguments().at(2).toString(),                       // parent_window
                                               message.arguments().at(3).toString(),                       // title
                                               choices,                                                    // options
                                               results);
        } else if (message.member() == QLatin1String("SaveFile")) {
            response = m_fileChooser->saveFile(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),  // handle
                                               message.arguments().at(1).toString(),                       // app_id
                                               message.arguments().at(2).toString(),                       // parent_window
                                               message.arguments().at(3).toString(),                       // title
                                               choices,                                                    // options
                                               results);
        }

        arguments << response;
        arguments << results;

    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Inhibit")) {
        if (message.member() == QLatin1String("Inhibit")) {
            QVariantMap options;

            QDBusArgument dbusArgument = message.arguments().at(4).value<QDBusArgument>();
            dbusArgument >> options;

            m_inhibit->inhibit(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),     // handle
                               message.arguments().at(1).toString(),                          // app_id
                               message.arguments().at(2).toString(),                          // window
                               message.arguments().at(3).toUInt(),                            // flags
                               options);                                                      // options
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Notification")) {
        if (message.member() == QLatin1String("AddNotification")) {
            QVariantMap notificationParams;

            QDBusArgument dbusArgument = message.arguments().at(2).value<QDBusArgument>();
            dbusArgument >> notificationParams;

            m_notification->addNotification(message.arguments().at(0).toString(),                           // app_id
                                            message.arguments().at(1).toString(),                           // id
                                            notificationParams);                                            // notification
        } else if (message.member() == QLatin1String("RemoveNotification")) {
            m_notification->removeNotification(message.arguments().at(0).toString(),                        // app_id
                                               message.arguments().at(1).toString());                       // id
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Print")) {
        uint response = 2;
        QVariantMap results;

        if (message.member() == QLatin1String("Print")) {
            QVariantMap options;

            QDBusArgument dbusArgument = message.arguments().at(5).value<QDBusArgument>();
            dbusArgument >> options;

            response = m_print->print(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),            // handle
                                      message.arguments().at(1).toString(),                                 // app_id
                                      message.arguments().at(2).toString(),                                 // parent_window
                                      message.arguments().at(3).toString(),                                 // title
                                      qvariant_cast<QDBusUnixFileDescriptor>(message.arguments().at(4)),    // fd
                                      options,                                                              // options
                                      results);
        } else if (message.member() == QLatin1String("PreparePrint")) {
            QVariantMap settings;
            QVariantMap pageSetup;
            QVariantMap options;

            QDBusArgument dbusArgument = message.arguments().at(4).value<QDBusArgument>();
            dbusArgument >> settings;

            QDBusArgument dbusArgument1 = message.arguments().at(5).value<QDBusArgument>();
            dbusArgument1 >> pageSetup;

            QDBusArgument dbusArgument2 = message.arguments().at(6).value<QDBusArgument>();
            dbusArgument2 >> options;

            response = m_print->preparePrint(qvariant_cast<QDBusObjectPath>(message.arguments().at(0)),     // handle
                                             message.arguments().at(1).toString(),                          // app_id
                                             message.arguments().at(2).toString(),                          // parent_window
                                             message.arguments().at(3).toString(),                          // title
                                             settings,                                                      // settings
                                             pageSetup,                                                     // page_setup
                                             options,                                                       // options
                                             results);
        }

        arguments << response;
        arguments << results;
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
            "<interface name=\"org.freedesktop.impl.portal.Email\">"
            "    <method name=\"ComposeEmail\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"window\" direction=\"in\"/>"
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
            "<interface name=\"org.freedesktop.impl.portal.Inhibit\">"
            "    <method name=\"Inhibit\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"window\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"flags\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
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
            "</interface>"
            "<interface name=\"org.freedesktop.impl.portal.Print\">"
            "    <method name=\"Print\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"title\" direction=\"in\"/>"
            "        <arg type=\"h\" name=\"fd\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"response\" direction=\"out\"/>"
            "        <arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
            "    </method>"
            "    <method name=\"PreparePrint\">"
            "        <arg type=\"o\" name=\"handle\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"app_id\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"parent_window\" direction=\"in\"/>"
            "        <arg type=\"s\" name=\"title\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"settings\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"page_setup\" direction=\"in\"/>"
            "        <arg type=\"a{sv}\" name=\"options\" direction=\"in\"/>"
            "        <arg type=\"u\" name=\"response\" direction=\"out\"/>"
            "        <arg type=\"a{sv}\" name=\"results\" direction=\"out\"/>"
            "    </method>"
            "</interface>");
    }

    qCDebug(XdgDesktopPortalKdeDesktopPortal) << nodes;

    return nodes;
}
