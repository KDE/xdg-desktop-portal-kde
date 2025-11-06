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
#include "session.h"

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


class GlobalShortcutsSession : public Session
{
    Q_OBJECT
public:
    explicit GlobalShortcutsSession(QObject *parent, const QString &appId, const QString &path);
    ~GlobalShortcutsSession() override;

    SessionType type() const override
    {
        return SessionType::GlobalShortcuts;
    }

    void setActions(const QList<ShortcutInfo> &shortcuts);
    void loadActions();

    QVariant shortcutDescriptionsVariant() const;
    Shortcuts shortcutDescriptions() const;
    KGlobalAccelComponentInterface *component() const
    {
        return m_component;
    }
    QString componentName() const
    {
        return m_appId.isEmpty() ? QLatin1String("token_") + m_token : m_appId;
    }

    QString appId() const
    {
        return m_appId;
    }

Q_SIGNALS:
    void shortcutsChanged();
    void shortcutActivated(const QString &shortcutName, qlonglong timestamp);
    void shortcutDeactivated(const QString &shortcutName, qlonglong timestamp);

private:
    const QString m_token;
    std::unordered_map<QString, std::unique_ptr<QAction>> m_shortcuts;
    KGlobalAccelInterface *const m_globalAccelInterface;
    KGlobalAccelComponentInterface *const m_component;
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
