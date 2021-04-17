/*
 * Copyright (c) 2020 Kai Uwe Broulik <kde@broulik.de>
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
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_NOTIFICATIONINHIBITION_H
#define XDG_DESKTOP_PORTAL_KDE_NOTIFICATIONINHIBITION_H

#include <QObject>

class NotificationInhibition : public QObject
{
    Q_OBJECT
public:
    explicit NotificationInhibition(const QString &appId, const QString &reason, QObject *parent = nullptr);
    ~NotificationInhibition() override;

private:
    static void uninhibit(uint cookie);
    uint m_cookie = 0;
};

#endif // XDG_DESKTOP_PORTAL_KDE_NOTIFICATIONINHIBITION_H
