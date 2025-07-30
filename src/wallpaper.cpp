// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

#include "wallpaper.h"

#include "dbushelpers.h"
#include "request.h"
#include "wallpaper_debug.h"

#include <KConfig>
#include <KConfigGroup>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace
{
void setDesktopBackground(const QUrl &url)
{
    for (unsigned int screenNumber = 0; screenNumber < qGuiApp->screens().size(); ++screenNumber) {
        auto message = QDBusMessage::createMethodCall("org.kde.plasmashell"_L1, "/PlasmaShell"_L1, "org.kde.PlasmaShell"_L1, "setWallpaper"_L1);
        message.setArguments({"org.kde.image"_L1, QVariantMap{{"Image"_L1, url.toString()}}, screenNumber});
        QDBusConnection::sessionBus().asyncCall(message);
    }
}

void setLockScreenBackground(const QUrl &url)
{
    KConfig screenLockerConfig("kscreenlockerrc"_L1);
    auto greeterGroup = screenLockerConfig.group(QString()).group("Greeter"_L1);
    greeterGroup.writeEntry("WallpaperPlugin", u"org.kde.image"_s);
    auto configGroup = greeterGroup.group(u"Wallpaper"_s).group(u"org.kde.image"_s).group(u"General"_s);
    configGroup.writeEntry("Image", url.toString());
}

void setWallpaper(const QUrl &url, WallpaperLocation::Location location)
{
    switch (location) {
    case WallpaperLocation::Desktop:
        setDesktopBackground(url);
        break;
    case WallpaperLocation::Lockscreen:
        setLockScreenBackground(url);
        break;
    case WallpaperLocation::Both:
        setDesktopBackground(url);
        setLockScreenBackground(url);
        break;
    }
}
}

void WallpaperPortal::SetWallpaperURI(const QDBusObjectPath &handle,
                                      const QString &app_id,
                                      const QString &parent_window,
                                      const QString &uri,
                                      const QVariantMap &options,
                                      const QDBusMessage &message,
                                      uint &replyResponse)
{
    qCDebug(XdgDesktopPortalKdeWallpaper) << "setWallpaperURI called with parameters:";
    qCDebug(XdgDesktopPortalKdeWallpaper) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeWallpaper) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeWallpaper) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeWallpaper) << "    uri: " << uri;
    qCDebug(XdgDesktopPortalKdeWallpaper) << "    options: " << options;

    const bool showPreview = options.value("show-preview"_L1, true).toBool();
    WallpaperLocation::Location location;
    if (const QString setOn = options.value("set-on"_L1).toString(); setOn == "background"_L1) {
        location = WallpaperLocation::Desktop;
    } else if (setOn == "lockscreen"_L1) {
        location = WallpaperLocation::Lockscreen;
    } else if (setOn == "both"_L1) {
        location = WallpaperLocation::Both;
    } else {
        qCWarning(XdgDesktopPortalKdeWallpaper) << "Dont know where to set wallpaper" << setOn;
        replyResponse = PortalResponse::OtherError;
        return;
    }

    const QUrl url(uri);
    if (!url.isValid()) {
        qCWarning(XdgDesktopPortalKdeWallpaper) << "Url is not valid" << uri;
        replyResponse = PortalResponse::OtherError;
    }

    // showPreview == false means the frontend already asked for us/remembered
    if (!showPreview) {
        setWallpaper(url, location);
        replyResponse = PortalResponse::Success;
        return;
    }
    auto dialog = new QuickDialog();
    dialog->create(u"WallpaperDialog"_s, {{u"location"_s, location}, {u"app"_s, Utils::applicationName(app_id)}, {u"image"_s, url}});
    Request::makeClosableDialogRequest(handle, dialog);
    delayReply(message, dialog, this, [location, url](DialogResult result) {
        if (result == DialogResult::Accepted) {
            setWallpaper(url, location);
        }
        return PortalResponse::fromDialogResult(result);
    });
}

#include "moc_wallpaper.cpp"
