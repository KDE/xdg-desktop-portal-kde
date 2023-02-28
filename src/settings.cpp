/*
 * SPDX-FileCopyrightText: 2018-2019 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018-2019 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 */

#include "settings.h"
#include "settings_debug.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QPalette>

#include <KConfigCore/KConfigGroup>

#include "desktopportal.h"

using namespace Qt::Literals::StringLiterals;

static bool groupMatches(const QString &group, const QStringList &patterns)
{
    return std::any_of(patterns.cbegin(), patterns.cend(), [&group](const auto &pattern) {
        if (pattern.isEmpty()) {
            return true;
        }

        if (pattern == group) {
            return true;
        }

        if (pattern.endsWith(QLatin1Char('*')) && group.startsWith(pattern.left(pattern.length() - 1))) {
            return true;
        }

        return false;
    });
}

class FdoAppearanceSettings : public SettingsModule
{
    Q_OBJECT
    static constexpr auto colorScheme = "color-scheme"_L1;

public:
    explicit FdoAppearanceSettings(QObject *parent = nullptr)
        : SettingsModule(parent)
    {
        connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this] {
            Q_EMIT settingChanged(group(), colorScheme, readFdoColorScheme());
        });
    }

    inline QString group() final
    {
        return u"org.freedesktop.appearance"_s;
    }

    VariantMapMap readAll(const QStringList &groups) final
    {
        Q_UNUSED(groups);
        VariantMapMap result;
        QVariantMap appearanceSettings;
        appearanceSettings.insert(colorScheme, readFdoColorScheme().variant());
        result.insert(group(), appearanceSettings);
        return result;
    }

    QVariant read(const QString &group, const QString &key) final
    {
        if (group == this->group() && key == colorScheme) {
            return QVariant::fromValue(readFdoColorScheme());
        }
        return {};
    }

private:
    QDBusVariant readFdoColorScheme()
    {
        const QPalette palette = QApplication::palette();
        const int windowBackgroundGray = qGray(palette.window().color().rgb());

        uint result = 0; // no preference

        if (windowBackgroundGray < 192) {
            result = 1; // prefer dark
        } else {
            result = 2; // prefer light
        }

        return QDBusVariant(result);
    }
};

class KDEGlobalsSettings : public SettingsModule
{
    Q_OBJECT

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

public:
    explicit KDEGlobalsSettings(QObject *parent = nullptr)
        : SettingsModule(parent)
    {
        m_kdeglobals = KSharedConfig::openConfig();

        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KDEPlatformTheme"),
                                              QStringLiteral("org.kde.KDEPlatformTheme"),
                                              QStringLiteral("refreshFonts"),
                                              this,
                                              SLOT(fontChanged()));
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KGlobalSettings"),
                                              QStringLiteral("org.kde.KGlobalSettings"),
                                              QStringLiteral("notifyChange"),
                                              this,
                                              // clang-format off
                                            SLOT(globalSettingChanged(int,int)));
        // clang-format on
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KToolBar"),
                                              QStringLiteral("org.kde.KToolBar"),
                                              QStringLiteral("styleChanged"),
                                              this,
                                              SLOT(toolbarStyleChanged()));
    }

    inline QString group() final
    {
        return u"org.kde.TabletMode"_s;
    }

    VariantMapMap readAll(const QStringList &groups) final
    {
        VariantMapMap result;

        const auto groupList = m_kdeglobals->groupList();
        for (const QString &settingGroupName : groupList) {
            // NOTE: use org.kde.kdeglobals prefix

            QString uniqueGroupName = QStringLiteral("org.kde.kdeglobals.") + settingGroupName;

            if (!groupMatches(uniqueGroupName, groups)) {
                continue;
            }

            QVariantMap map;
            KConfigGroup configGroup(m_kdeglobals, settingGroupName);

            const auto keyList = configGroup.keyList();
            for (const QString &key : keyList) {
                map.insert(key, configGroup.readEntry(key));
            }

            result.insert(uniqueGroupName, map);
        }

        return result;
    }

    QVariant read(const QString &group, const QString &key) final
    {
        return readProperty(group, key).variant();
    }

private Q_SLOTS:
    void fontChanged()
    {
        Q_EMIT settingChanged(u"org.kde.kdeglobals.General"_s, u"font"_s, readProperty(u"org.kde.kdeglobals.General"_s, u"font"_s));
    }

    void globalSettingChanged(int type, int arg)
    {
        m_kdeglobals->reparseConfiguration();

        // Mostly based on plasma-integration needs
        switch (type) {
        case PaletteChanged:
            // Plasma-integration will be loading whole palette again, there is no reason to try to identify
            // particular categories or colors
            Q_EMIT settingChanged(u"org.kde.kdeglobals.General"_s, u"ColorScheme"_s, readProperty(u"org.kde.kdeglobals.General"_s, u"ColorScheme"_s));
            break;
        case FontChanged:
            fontChanged();
            break;
        case StyleChanged:
            Q_EMIT settingChanged(u"org.kde.kdeglobals.KDE"_s, u"widgetStyle"_s, readProperty(u"org.kde.kdeglobals.KDE"_s, u"widgetStyle"_s));
            break;
        case SettingsChanged: {
            auto category = SettingsCategory(arg);
            if (category == SETTINGS_QT || category == SETTINGS_MOUSE) {
                // TODO
            } else if (category == SETTINGS_STYLE) {
                // TODO
            }
            break;
        }
        case IconChanged:
            // we will get notified about each category, but it probably makes sense to send this signal just once
            if (arg == 0) { // KIconLoader::Desktop
                Q_EMIT settingChanged(u"org.kde.kdeglobals.Icons"_s, u"Theme"_s, readProperty(u"org.kde.kdeglobals.Icons"_s, u"Theme"_s));
            }
            break;
        case CursorChanged:
            // TODO
            break;
        case ToolbarStyleChanged:
            toolbarStyleChanged();
            break;
        default:
            break;
        }
    }

    void toolbarStyleChanged()
    {
        Q_EMIT settingChanged(u"org.kde.kdeglobals.Toolbar style"_s,
                              u"ToolButtonStyle"_s,
                              readProperty(u"org.kde.kdeglobals.Toolbar style"_s, u"ToolButtonStyle"_s));
    }

private:
    QDBusVariant readProperty(const QString &group, const QString &key)
    {
        static constexpr auto prefixLength = "org.kde.kdeglobals."_L1.length();
        QString groupName = group.right(group.length() - prefixLength);

        if (!m_kdeglobals->hasGroup(groupName)) {
            qCWarning(XdgDesktopPortalKdeSettings) << "Group " << group << " doesn't exist";
            return {};
        }

        KConfigGroup configGroup(m_kdeglobals, groupName);

        if (!configGroup.hasKey(key)) {
            qCWarning(XdgDesktopPortalKdeSettings) << "Key " << key << " doesn't exist";
            return {};
        }

        return QDBusVariant(configGroup.readEntry(key));
    }

    KSharedConfigPtr m_kdeglobals = KSharedConfig::openConfig();
};

SettingsPortal::SettingsPortal(DesktopPortal *parent)
    : QDBusAbstractAdaptor(parent)
    , m_parent(parent)
{
    m_settings.push_back(std::make_unique<FdoAppearanceSettings>(this));
    m_settings.push_back(std::make_unique<KDEGlobalsSettings>(this));
    for (const auto &setting : std::as_const(m_settings)) {
        connect(setting.get(), &SettingsModule::settingChanged, this, &SettingsPortal::SettingChanged);
    }
    qDBusRegisterMetaType<VariantMapMap>();
}

void SettingsPortal::ReadAll(const QStringList &groups)
{
    qCDebug(XdgDesktopPortalKdeSettings) << "ReadAll called with parameters:";
    qCDebug(XdgDesktopPortalKdeSettings) << "    groups: " << groups;

    VariantMapMap result;

    for (const auto &setting : m_settings) {
        if (groupMatches(setting->group(), groups)) {
            result.insert(setting->readAll(groups));
        }
    }

    QDBusMessage message = m_parent->message();
    QDBusMessage reply = message.createReply(QVariant::fromValue(result));
    QDBusConnection::sessionBus().send(reply);
}

void SettingsPortal::Read(const QString &group, const QString &key)
{
    qCDebug(XdgDesktopPortalKdeSettings) << "Read called with parameters:";
    qCDebug(XdgDesktopPortalKdeSettings) << "    group: " << group;
    qCDebug(XdgDesktopPortalKdeSettings) << "    key: " << key;

    QDBusMessage message = m_parent->message();

    const auto sentMesssage = std::any_of(m_settings.cbegin(), m_settings.cend(), [&message, &group, &key](const auto &setting) {
        if (group.startsWith(setting->group())) {
            const QVariant result = setting->read(group, key);
            QDBusMessage reply;
            if (result.isNull()) {
                reply = message.createErrorReply(QDBusError::UnknownProperty, QStringLiteral("Property doesn't exist"));
            } else {
                reply = message.createReply(QVariant::fromValue(QDBusVariant(result)));
            }
            QDBusConnection::sessionBus().send(reply);
            return true;
        }
        return false;
    });
    if (sentMesssage) {
        return;
    }

    qCWarning(XdgDesktopPortalKdeSettings) << "Namespace " << group << " is not supported";
    QDBusMessage reply = message.createErrorReply(QDBusError::UnknownProperty, QStringLiteral("Namespace is not supported"));
    QDBusConnection::sessionBus().send(reply);
}

#include "settings.moc"
