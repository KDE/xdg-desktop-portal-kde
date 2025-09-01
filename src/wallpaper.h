// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <qqmlintegration.h>

class QDBusMessage;

namespace WallpaperLocation
{
Q_NAMESPACE
QML_ELEMENT
enum Location {
    Desktop,
    Lockscreen,
    Both
};
Q_ENUM_NS(Location)
}

class WallpaperPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Wallpaper")
public:
    explicit WallpaperPortal(QObject *parent = nullptr)
        : QDBusAbstractAdaptor(parent)
    {
    }
public Q_SLOTS:
    void SetWallpaperURI(const QDBusObjectPath &handle,
                         const QString &app_id,
                         const QString &parent_window,
                         const QString &uri,
                         const QVariantMap &options,
                         const QDBusMessage &message,
                         uint &replyResponse);
};
