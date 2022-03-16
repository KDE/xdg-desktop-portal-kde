/*
 * SPDX-FileCopyrightText: 2016 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016 Jan Grulich <jgrulich@redhat.com>
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
    ~DesktopPortal() override;

private:
    AccessPortal *const m_access;
    AccountPortal *const m_account;
    AppChooserPortal *const m_appChooser;
    BackgroundPortal *m_background = nullptr;
    EmailPortal *const m_email;
    FileChooserPortal *const m_fileChooser;
    InhibitPortal *const m_inhibit;
    NotificationPortal *const m_notification;
    PrintPortal *const m_print;
    ScreenshotPortal *m_screenshot = nullptr;
    SettingsPortal *const m_settings;
    ScreenCastPortal *m_screenCast = nullptr;
    RemoteDesktopPortal *m_remoteDesktop = nullptr;
};

#endif // XDG_DESKTOP_PORTAL_KDE_DESKTOP_PORTAL_H
