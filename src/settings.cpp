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

#include "settings.h"

#include <QDBusMetaType>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusConnection>

#include <QLoggingCategory>

#include <KConfigCore/KConfigGroup>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeSettings, "xdp-kde-settings")

Q_DECLARE_METATYPE(SettingsPortal::VariantMapMap)

QDBusArgument &operator<<(QDBusArgument &argument, const SettingsPortal::VariantMapMap &mymap)
{
    argument.beginMap(QVariant::String, QVariant::Map);

    QMapIterator<QString, QVariantMap> i(mymap);
    while (i.hasNext()) {
        i.next();
        argument.beginMapEntry();
        argument << i.key() << i.value();
        argument.endMapEntry();
    }
    argument.endMap();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SettingsPortal::VariantMapMap &mymap)
{
    argument.beginMap();
    mymap.clear();

    while (!argument.atEnd()) {
        QString key;
        QVariantMap value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        mymap.insert(key, value);
    }

    argument.endMap();
    return argument;
}

static bool groupMatches(const QString &group, const QStringList &patterns)
{
    for (const QString &pattern : patterns) {
        if (pattern.isEmpty()) {
            return true;
        }

        if (pattern == group) {
            return true;
        }

        if (pattern.endsWith(QLatin1Char('*')) && group.startsWith(pattern.left(pattern.length() - 1))) {
            return true;
        }
    }

    return false;
}

SettingsPortal::SettingsPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<VariantMapMap>();

    m_kdeglobals = KSharedConfig::openConfig();
}

SettingsPortal::~SettingsPortal()
{
}

void SettingsPortal::ReadAll(const QStringList &groups)
{
    qCDebug(XdgDesktopPortalKdeSettings) << "ReadAll called with parameters:";
    qCDebug(XdgDesktopPortalKdeSettings) << "    groups: " << groups;

    //FIXME this is super ugly, but I was unable to make it properly return VariantMapMap
    QObject *obj = QObject::parent();

    if (!obj) {
        qCWarning(XdgDesktopPortalKdeSettings) << "Failed to get dbus context";
        return;
    }

    void *ptr = obj->qt_metacast("QDBusContext");
    QDBusContext *q_ptr = reinterpret_cast<QDBusContext *>(ptr);

    if (!q_ptr) {
        qCWarning(XdgDesktopPortalKdeSettings) << "Failed to get dbus context";
        return;
    }

    VariantMapMap result;
    for (const QString &settingGroupName : m_kdeglobals->groupList()) {
        //NOTE: use org.kde.kdeglobals prefix

        QString uniqueGroupName = QStringLiteral("org.kde.kdeglobals.") + settingGroupName;

        if (!groupMatches(uniqueGroupName, groups)) {
            continue;
        }

        QVariantMap map;
        KConfigGroup configGroup(m_kdeglobals, settingGroupName);

        for (const QString &key : configGroup.keyList()) {
            map.insert(key, configGroup.readEntry(key));
        }

        result.insert(uniqueGroupName, map);
    }

    QDBusMessage message = q_ptr->message();
    QDBusMessage reply = message.createReply(QVariant::fromValue(result));
    QDBusConnection::sessionBus().send(reply);
}

QDBusVariant SettingsPortal::Read(const QString &group, const QString &key)
{
    qCDebug(XdgDesktopPortalKdeSettings) << "Read called with parameters:";
    qCDebug(XdgDesktopPortalKdeSettings) << "    group: " << group;
    qCDebug(XdgDesktopPortalKdeSettings) << "    key: " << key;

    // All our namespaces start with this prefix
    if (!group.startsWith(QStringLiteral("org.kde.kdeglobals"))) {
        qCWarning(XdgDesktopPortalKdeSettings) << "Namespace " << group << " is not supported";
        return QDBusVariant();
    }

    QString groupName = group.right(group.length() - QStringLiteral("org.kde.kdeglobals.").length());

    if (!m_kdeglobals->hasGroup(groupName)) {
        qCWarning(XdgDesktopPortalKdeSettings) << "Group " << group << " doesn't exist";
        return QDBusVariant();
    }

    KConfigGroup configGroup(m_kdeglobals, groupName);

    if (!configGroup.hasKey(key)) {
        qCWarning(XdgDesktopPortalKdeSettings) << "Key " << key << " doesn't exist";
        return QDBusVariant();
    }

    return QDBusVariant(configGroup.readEntry(key));
}
