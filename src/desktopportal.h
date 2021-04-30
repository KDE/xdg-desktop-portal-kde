/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
#define XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H

#include <QDBusContext>
#include <QObject>

#include "access.h"
#include "account.h"
#include "appchooser.h"
#include "background.h"
#include "email.h"
#include "filechooser.h"
#include "inhibit.h"
#include "notification.h"
#include "print.h"
#include "screenshot.h"
#include "settings.h"
#include "waylandintegration.h"

#include "remotedesktop.h"
#include "screencast.h"

class DesktopPortal : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit DesktopPortal(QObject *parent = nullptr);
    ~DesktopPortal();

private:
    AccessPortal *m_access;
    AccountPortal *m_account;
    AppChooserPortal *m_appChooser;
    BackgroundPortal *m_background;
    EmailPortal *m_email;
    FileChooserPortal *m_fileChooser;
    InhibitPortal *m_inhibit;
    NotificationPortal *m_notification;
    PrintPortal *m_print;
    ScreenshotPortal *m_screenshot;
    SettingsPortal *m_settings;
    ScreenCastPortal *m_screenCast;
    RemoteDesktopPortal *m_remoteDesktop;
};

#endif // XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
