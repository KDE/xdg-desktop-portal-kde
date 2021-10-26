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

#include <KConfigCore/KSharedConfig>

class SettingsPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Settings")
    Q_PROPERTY(uint version READ version CONSTANT)
public:
    /**
     * An identifier for change signals.
     * @note Copied from KGlobalSettings
     */
    enum ChangeType {
        PaletteChanged = 0,
        FontChanged,
        StyleChanged,
        SettingsChanged,
        IconChanged,
        CursorChanged,
        ToolbarStyleChanged,
        ClipboardConfigChanged,
        BlockShortcuts,
        NaturalSortingChanged,
    };

    /**
     * Valid values for the settingsChanged signal
     * @note Copied from KGlobalSettings
     */
    enum SettingsCategory {
        SETTINGS_MOUSE,
        SETTINGS_COMPLETION,
        SETTINGS_PATHS,
        SETTINGS_POPUPMENU,
        SETTINGS_QT,
        SETTINGS_SHORTCUTS,
        SETTINGS_LOCALE,
        SETTINGS_STYLE,
    };

    explicit SettingsPortal(QObject *parent);
    ~SettingsPortal() override;

    typedef QMap<QString, QMap<QString, QVariant>> VariantMapMap;

    uint version() const
    {
        return 1;
    }

public Q_SLOTS:
    void ReadAll(const QStringList &groups);
    void Read(const QString &group, const QString &key);

Q_SIGNALS:
    void SettingChanged(const QString &group, const QString &key, const QDBusVariant &value);

private Q_SLOTS:
    void fontChanged();
    void globalSettingChanged(int type, int arg);
    void toolbarStyleChanged();

private:
    QDBusVariant readProperty(const QString &group, const QString &key);
    QDBusVariant readFdoColorScheme();

    KSharedConfigPtr m_kdeglobals;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SETTINGS_H
