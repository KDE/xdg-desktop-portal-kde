/*
 * SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "globalshortcuts.h"
#include "quickdialog.h"
#include "request.h"
#include "session.h"
#include "utils.h"
#include "xdgshortcut.h"

#include <KGlobalAccel>
#include <KLocalizedString>
#include <KStandardShortcut>

#include <QAbstractListModel>
#include <QAction>
#include <QDBusMetaType>
#include <QDataStream>
#include <QDesktopServices>
#include <QLoggingCategory>
#include <QUrl>
#include <QWindow>

#include <QQmlEngine>

#include <ranges>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeGlobalShortcuts, "xdp-kde-GlobalShortcuts")

using namespace Qt::StringLiterals;

class ShortcutsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasGlobalConflict READ hasGlobalConflict NOTIFY hasGlobalConflictChanged)
public:
    enum class Roles {
        KeySequence = Qt::UserRole,
        GlobalConflict,
        StandardConcflict,
        ConflictText,
    };
    ShortcutsModel(QList<ShortcutInfo> &shortcuts)
        : m_shortcuts(shortcuts)
    {
        m_standardConflicts.reserve(shortcuts.size());
        m_globalConflicts.reserve(shortcuts.size());
        for (const auto &shortcut : m_shortcuts) {
            m_standardConflicts.push_back(KStandardShortcut::find(shortcut.keySequence));
            m_globalConflicts.push_back(!KGlobalAccel::isGlobalShortcutAvailable(shortcut.keySequence));
        }
    }
    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::DisplayRole, "display"},
                {qToUnderlying(Roles::KeySequence), "keySequence"},
                {qToUnderlying(Roles::GlobalConflict), "globalConflict"},
                {qToUnderlying(Roles::StandardConcflict), "standardConflict"},
                {qToUnderlying(Roles::ConflictText), "conflictText"}};
    }
    int rowCount([[maybe_unused]] const QModelIndex &parent) const override
    {
        return m_shortcuts.size();
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
            return QVariant();
        }
        auto &shortcut = m_shortcuts.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return shortcut.description;
        case qToUnderlying(Roles::KeySequence):
            return shortcut.keySequence;
        case qToUnderlying(Roles::GlobalConflict):
            return m_globalConflicts.at(index.row());
        case qToUnderlying(Roles::StandardConcflict):
            return m_standardConflicts.at(index.row()) != KStandardShortcut::AccelNone;
        case qToUnderlying(Roles::ConflictText):
            if (m_globalConflicts.at(index.row())) {
                auto takenBy = KGlobalAccel::globalShortcutsByKey(shortcut.keySequence);
                if (takenBy.empty()) {
                    return QString();
                }
                return xi18nc("@info:tooltip",
                              "The shortcut is already assigned to the action <interface>%1</interface> of <application>%2</application>."
                              "Re-enter it here to override that action.",
                              takenBy[0].friendlyName(),
                              takenBy[0].componentFriendlyName());
            }
            if (m_standardConflicts.at(index.row()) != KStandardShortcut::AccelNone) {
                return xi18nc("@info:tooltip",
                              "The shortcut is already assigned to the <interface>%1</interface> standard action.",
                              KStandardShortcut::name(m_standardConflicts.at(index.row())));
            }
            break;
        }
        return QVariant();
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
            if (role == qToUnderlying(Roles::KeySequence)) {
                const auto sequence = value.value<QKeySequence>();
                m_shortcuts[index.row()].keySequence = sequence;
                m_globalConflicts[index.row()] = !KGlobalAccel::isGlobalShortcutAvailable(sequence);
                m_standardConflicts[index.row()] = KStandardShortcut::find(sequence);
                Q_EMIT dataChanged(index,
                                   index,
                                   {qToUnderlying(Roles::KeySequence),
                                    qToUnderlying(Roles::GlobalConflict),
                                    qToUnderlying(Roles::StandardConcflict),
                                    qToUnderlying(Roles::ConflictText)});
                Q_EMIT hasGlobalConflictChanged();
                return true;
            }
        }
        return false;
    }
    bool hasGlobalConflict()
    {
        return std::ranges::any_of(m_globalConflicts, std::identity());
    }
Q_SIGNALS:
    void hasGlobalConflictChanged();

private:
    QList<ShortcutInfo> &m_shortcuts;
    QList<bool> m_globalConflicts;
    QList<KStandardShortcut::StandardShortcut> m_standardConflicts;
};

GlobalShortcutsPortal::GlobalShortcutsPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<Shortcut>();
    qDBusRegisterMetaType<Shortcuts>();

    Q_ASSERT(QLatin1String(QDBusMetaType::typeToSignature(QMetaType(qMetaTypeId<Shortcuts>()))) == QLatin1String("a(sa{sv})"));
}

GlobalShortcutsPortal::~GlobalShortcutsPortal() = default;

uint GlobalShortcutsPortal::version() const
{
    return 1;
}

uint GlobalShortcutsPortal::CreateSession(const QDBusObjectPath &handle,
                                          const QDBusObjectPath &session_handle,
                                          const QString &app_id,
                                          const QVariantMap &options,
                                          QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    options: " << options;

    auto session = qobject_cast<GlobalShortcutsSession *>(Session::createSession(this, Session::GlobalShortcuts, app_id, session_handle.path()));
    if (!session) {
        return 2;
    }

    session->loadActions();

    connect(session, &GlobalShortcutsSession::shortcutsChanged, this, [this, session, session_handle] {
        Q_EMIT ShortcutsChanged(session_handle, session->shortcutDescriptions());
    });

    connect(session, &GlobalShortcutsSession::shortcutActivated, this, [this, session](const QString &shortcutName, qlonglong timestamp) {
        Q_EMIT Activated(QDBusObjectPath(session->handle()), shortcutName, timestamp);
    });
    connect(session, &GlobalShortcutsSession::shortcutDeactivated, this, [this, session](const QString &shortcutName, qlonglong timestamp) {
        Q_EMIT Deactivated(QDBusObjectPath(session->handle()), shortcutName, timestamp);
    });

    Q_UNUSED(results);
    return 0;
}

uint GlobalShortcutsPortal::ListShortcuts(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "ListShortcuts called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();

    auto session = qobject_cast<GlobalShortcutsSession *>(Session::getSession(session_handle.path()));
    if (!session) {
        return 2;
    }
    results = {
        {u"shortcuts"_s, session->shortcutDescriptionsVariant()},
    };
    return 0;
}

uint GlobalShortcutsPortal::BindShortcuts(const QDBusObjectPath &handle,
                                          const QDBusObjectPath &session_handle,
                                          const Shortcuts &shortcuts,
                                          const QString &parent_window,
                                          const QVariantMap &options,
                                          QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "BindShortcuts called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    shortcuts: " << shortcuts;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    options: " << options;

    auto session = qobject_cast<GlobalShortcutsSession *>(Session::getSession(session_handle.path()));
    if (!session) {
        return 2;
    }

    auto previousShortcuts = session->shortcutDescriptions();
    auto currentShortcuts = shortcuts;
    std::ranges::sort(currentShortcuts | std::views::keys);
    std::ranges::sort(previousShortcuts | std::views::keys);

    Shortcuts newShortcuts;
    std::ranges::set_difference(currentShortcuts, previousShortcuts, std::back_inserter(newShortcuts), {}, &Shortcut::first, &Shortcut::first);
    Shortcuts returningShortcuts;
    std::ranges::set_intersection(currentShortcuts, previousShortcuts, std::back_inserter(returningShortcuts), {}, &Shortcut::first, &Shortcut::first);

    auto toShortcutInfo = [](const Shortcut &shortcut) {
        return ShortcutInfo{.id = shortcut.first,
                            .description = shortcut.second.value(u"description"_s).toString(),
                            .keySequence = XdgShortcut::parse(shortcut.second.value(u"preferred_trigger"_s).toString()).value_or(QKeySequence())};
    };

    QList<ShortcutInfo> newShortcutInfos;
    newShortcutInfos.reserve(newShortcuts.size());
    std::ranges::transform(newShortcuts, std::back_inserter(newShortcutInfos), toShortcutInfo);

    QList<ShortcutInfo> returningShortcutInfos;
    returningShortcutInfos.reserve(returningShortcuts.size());
    std::ranges::transform(returningShortcuts, std::back_inserter(returningShortcutInfos), toShortcutInfo);

    if (!newShortcutInfos.empty()) {
        auto dialog = QuickDialog();
        Utils::setParentWindow(dialog.windowHandle(), parent_window);
        Request::makeClosableDialogRequestWithSession(handle, &dialog, session);
        ShortcutsModel model(newShortcutInfos);
        dialog.create(u"qrc:/GlobalShortcutsDialog.qml"_s,
                      {{u"app"_s, Utils::applicationName(session->appId())},
                       {u"returningShortcuts"_s, QVariant::fromValue(returningShortcutInfos)},
                       {u"newShortcuts"_s, QVariant::fromValue(&model)},
                       {u"component"_s, session->componentName()}});
        auto result = dialog.exec();
        if (!result) {
            session->setActions({});
            return 1;
        } else {
            QList<ShortcutInfo> currentShortcutInfos;
            currentShortcutInfos.reserve(currentShortcuts.size());
            std::ranges::set_union(newShortcutInfos,
                                   returningShortcutInfos,
                                   std::back_inserter(currentShortcutInfos),
                                   {},
                                   &ShortcutInfo::id,
                                   &ShortcutInfo::id);
            session->setActions(currentShortcutInfos);
        }
    }

    results = {
        {u"shortcuts"_s, session->shortcutDescriptionsVariant()},
    };
    return 0;
}

#include "globalshortcuts.moc"
