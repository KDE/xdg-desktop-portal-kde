/*
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QKeySequence>
#include <QProperty>
#include <qqmlintegration.h>

#include "dbushelpers.h"

struct ShortcutInfo {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QKeySequence keySequence MEMBER keySequence)
public:
    QString id;
    QString description;
    QKeySequence keySequence;
    QKeySequence preferredKeySequence;
};

class GlobalShortcutsPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_DISABLE_COPY(GlobalShortcutsPortal)
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.GlobalShortcuts")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    explicit GlobalShortcutsPortal(QObject *parent);
    ~GlobalShortcutsPortal() override;

    uint version() const;

public Q_SLOTS:
    void BindShortcuts(const QDBusObjectPath &handle,
                       const QDBusObjectPath &session_handle,
                       const Shortcuts &shortcuts,
                       const QString &parent_window,
                       const QVariantMap &options,
                       const QDBusMessage &message,
                       uint &replyResponse,
                       QVariantMap &replyResults);
    uint CreateSession(const QDBusObjectPath &handle,
                       const QDBusObjectPath &session_handle,
                       const QString &app_id,
                       const QVariantMap &options,
                       QVariantMap &results);
    uint ListShortcuts(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, QVariantMap &results);
    void ConfigureShortcuts(const QDBusObjectPath &session_handle, const QString &parent_window, const QVariantMap &options);

Q_SIGNALS:
    void Activated(const QDBusObjectPath &session_handle, const QString &shortcutId, quint64 timestamp, const QVariantMap &unused = {});
    void Deactivated(const QDBusObjectPath &session_handle, const QString &shortcutId, quint64 timestamp, const QVariantMap &unused = {});
    void ShortcutsChanged(const QDBusObjectPath &session_handle, const Shortcuts &shortcuts);
};
