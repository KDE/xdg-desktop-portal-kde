// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

class QDBusMessage;

class WallpaperPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Wallpaper")
public:
    explicit WallpaperPortal(QObject *parent = nullptr)
        : QDBusAbstractAdaptor(parent)
    {
    }
    enum class WallpaperLocation {
        Desktop,
        Lockscreen,
        Both
    };

public Q_SLOTS:
    void SetWallpaperURI(const QDBusObjectPath &handle,
                         const QString &app_id,
                         const QString &parent_window,
                         const QString &uri,
                         const QVariantMap &options,
                         const QDBusMessage &message,
                         uint &replyResponse);
};
