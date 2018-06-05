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

#ifndef XDG_DESKTOP_PORTAL_KDE_SESSION_H
#define XDG_DESKTOP_PORTAL_KDE_SESSION_H

#include <QObject>
#include <QDBusVirtualObject>

class Session : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit Session(QObject *parent = nullptr, const QString &appId = QString(), const QString &path = QString());
    ~Session();

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;
    QString introspect(const QString &path) const override;

    bool close();

    bool multipleSources() const;
    void setMultipleSources(bool multipleSources);

Q_SIGNALS:
    void closed();

private:
    bool m_multipleSources;
    const QString m_appId;
    const QString m_path;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SESSION_H

