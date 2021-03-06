/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
 */

#include "desktopportal.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeDesktopPortal, "xdp-kde-desktop-portal")

DesktopPortal::DesktopPortal(QObject *parent)
    : QObject(parent)
    , m_access(new AccessPortal(this))
    , m_account(new AccountPortal(this))
    , m_appChooser(new AppChooserPortal(this))
    , m_email(new EmailPortal(this))
    , m_fileChooser(new FileChooserPortal(this))
    , m_inhibit(new InhibitPortal(this))
    , m_notification(new NotificationPortal(this))
    , m_print(new PrintPortal(this))
    , m_settings(new SettingsPortal(this))
{
    const QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP").toUpper();
    if (xdgCurrentDesktop == "KDE") {
        m_background = new BackgroundPortal(this);
        m_screenCast = new ScreenCastPortal(this);
        m_remoteDesktop = new RemoteDesktopPortal(this);
        m_screenshot = new ScreenshotPortal(this);
        WaylandIntegration::init();
    }
}

DesktopPortal::~DesktopPortal()
{
}
