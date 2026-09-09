/*
 * SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "../src/settings.h"

#include <QStandardPaths>
#include <QTest>

#include <KConfig>
#include <KConfigGroup>

using namespace Qt::StringLiterals;

auto sorted(auto list)
{
    std::ranges::sort(list);
    return list;
};

static auto expectedVirtualKeyboardKeys =
    sorted(QStringList{u"active"_s, u"activeClientSupportsTextInput"_s, u"available"_s, u"visible"_s, u"willShowOnActive"_s});
static auto expectedTabletmodeKeys = sorted(QStringList{u"enabled"_s, u"available"_s});
static auto expectedFdoKeys = sorted(QStringList{u"color-scheme"_s, u"accent-color"_s, u"reduced-motion"_s});

class SettingsPortalTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void readAll()
    {
        SettingsPortal portal(nullptr);

        const auto settings = portal.ReadAll({u""_s});

        QCOMPARE(sorted(settings.value(u"org.freedesktop.appearance"_s).keys()), expectedFdoKeys);
        QCOMPARE(sorted(settings.value(u"org.kde.VirtualKeyboard"_s).keys()), expectedVirtualKeyboardKeys);
        QCOMPARE(sorted(settings.value(u"org.kde.TabletMode"_s).keys()), expectedTabletmodeKeys);
        auto filtered = settings.keys() | std::views::filter([](const QString &key) {
                            return key.startsWith(u"org.kde.kdeglobals"_s);
                        });
        std::vector<QString> kdeglobals(std::from_range, filtered);
        QCOMPARE_NE(kdeglobals, std::vector<QString>());

        const auto settings2 = portal.ReadAll({});

        QCOMPARE(settings2, settings);
    }

    void readAllWithSpecificGroup()
    {
        SettingsPortal portal(nullptr);

        const auto settings = portal.ReadAll({u"org.freedesktop.appearance"_s});

        QCOMPARE(sorted(settings.value(u"org.freedesktop.appearance"_s).keys()), expectedFdoKeys);
    }

    void readAllWithGlob()
    {
        auto globals = KSharedConfig::openConfig();
        globals->group(u"Test1"_s).writeEntry("Foo", "Bar");
        globals->group(u"KDECommunity"_s).writeEntry("is", "awesome");
        globals->sync();

        SettingsPortal portal(nullptr);

        const auto settings = portal.ReadAll({u"org.kde.kdeglobals.*"_s});

        QVERIFY(settings.contains(u"org.kde.kdeglobals.Test1"_s));
        QVERIFY(settings.contains(u"org.kde.kdeglobals.KDECommunity"_s));
        QCOMPARE(settings[u"org.kde.kdeglobals.Test1"_s], QVariantMap({{u"Foo"_s, u"Bar"_s}}));
        QCOMPARE(settings[u"org.kde.kdeglobals.KDECommunity"_s], QVariantMap({{u"is"_s, u"awesome"_s}}));
    }

    void readKdeGlobalsSubgroup()
    {
        auto globals = KSharedConfig::openConfig();
        globals->group(u"General"_s).writeEntry("SettingsPortalTest", "value");
        globals->sync();

        SettingsPortal portal(nullptr);

        QCOMPARE(portal.Read(u"org.kde.kdeglobals.General"_s, u"SettingsPortalTest"_s).variant().toString(), u"value"_s);
    }
};

QTEST_MAIN(SettingsPortalTest)

#include "settingsportaltest.moc"
