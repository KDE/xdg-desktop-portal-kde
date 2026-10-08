/*
 * SPDX-FileCopyrightText: 2018-2019 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018-2019 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SETTINGS_H
#define XDG_DESKTOP_PORTAL_KDE_SETTINGS_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include <KSharedConfig>

#include "dbushelpers.h"

#include <functional>

class QDBusContext;
class FdoAppearanceSettings;
class KDEGlobalsSettings;
class SettingsModule;

#if defined __cpp_lib_move_only_function && __cpp_lib_move_only_function >= 202110L
template<typename... T>
using function = std::move_only_function<T...>;
#else
template<typename... T>
using function = std::function<T...>;
#endif

class SettingsModule : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~SettingsModule() override = default;
    Q_DISABLE_COPY_MOVE(SettingsModule)
    virtual inline QString group() = 0;
    virtual VariantMapMap readAll(const QStringList &groups) = 0;
    virtual QVariant read(const QString &group, const QString &key) = 0;
    Q_SIGNAL void settingChanged(const QString &group, const QString &key, const QDBusVariant &value);
};

class SettingsPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Settings")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    explicit SettingsPortal(QObject *parent, function<void(const QDBusError &)> errorSender);

    uint version() const
    {
        return 1;
    }

public Q_SLOTS:
    VariantMapMap ReadAll(const QStringList &groups);
    QDBusVariant Read(const QString &group, const QString &key);

Q_SIGNALS:
    void SettingChanged(const QString &group, const QString &key, const QDBusVariant &value);

private:
    function<void(const QDBusError &)> m_errorSender;
    std::vector<std::unique_ptr<SettingsModule>> m_settings;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SETTINGS_H
