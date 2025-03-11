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
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KStandardShortcut>

#include <QAbstractListModel>
#include <QAction>
#include <QDBusMetaType>
#include <QDataStream>
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
    Q_PROPERTY(bool hasInternalConflict READ hasInternalConflict NOTIFY hasInternalConflictChanged)
public:
    enum Roles {
        KeySequence = Qt::UserRole,
        GlobalConflict,
        StandardConflict,
        InternalConflict,
        ConflictText,
    };
    ShortcutsModel(const QList<ShortcutInfo> &shortcuts, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_shortcuts(shortcuts)
    {
        m_standardConflicts.reserve(shortcuts.size());
        m_globalConflicts.reserve(shortcuts.size());
        for (const auto &shortcut : m_shortcuts) {
            m_standardConflicts.push_back(KStandardShortcut::find(shortcut.keySequence));
            m_globalConflicts.push_back(!KGlobalAccel::isGlobalShortcutAvailable(shortcut.keySequence));
            m_internalConflictDetector.insert(shortcut.keySequence, m_internalConflictDetector.size());
        }
    }
    const QList<ShortcutInfo> &shortcuts()
    {
        return m_shortcuts;
    }
    QHash<int, QByteArray> roleNames() const override
    {
        return {{Qt::DisplayRole, "display"},
                {Roles::KeySequence, "keySequence"},
                {Roles::GlobalConflict, "globalConflict"},
                {Roles::StandardConflict, "standardConflict"},
                {Roles::InternalConflict, "internalConflict"},
                {Roles::ConflictText, "conflictText"}};
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
        case Roles::KeySequence:
            return shortcut.keySequence;
        case Roles::GlobalConflict:
            return m_globalConflicts.at(index.row());
        case Roles::StandardConflict:
            return m_standardConflicts.at(index.row()) != KStandardShortcut::AccelNone;
        case Roles::InternalConflict:
            return !shortcut.keySequence.isEmpty() && m_internalConflictDetector.count(shortcut.keySequence) > 1;
        case Roles::ConflictText:
            if (m_globalConflicts.at(index.row())) {
                auto takenBy = KGlobalAccel::globalShortcutsByKey(shortcut.keySequence);
                if (takenBy.empty()) {
                    return QString();
                }
                return xi18nc("@info:tooltip",
                              "The shortcut is already assigned to the action <interface>%1</interface> of <application>%2</application>. "
                              "Re-enter it here to override that action.",
                              takenBy[0].friendlyName(),
                              takenBy[0].componentFriendlyName());
            }
            if (m_standardConflicts.at(index.row()) != KStandardShortcut::AccelNone) {
                return xi18nc("@info:tooltip",
                              "The shortcut is already assigned to the <interface>%1</interface> standard action.",
                              KStandardShortcut::name(m_standardConflicts.at(index.row())));
            }
            if (shortcut.keySequence == QKeySequence()) {
                break;
            }
            if (auto conflicts = m_internalConflictDetector.values(shortcut.keySequence); conflicts.size() != 1) {
                QStringList descriptions;
                std::ranges::sort(conflicts); // Want same order as the main view
                for (const auto index : conflicts) {
                    descriptions.push_back(m_shortcuts.at(index).description);
                }
                return xi18nc("@info:tooltip %1 is a list of actions",
                              "The shortcut is assigned to multiple actions (<interface>%1</interface>)",
                              QLocale().createSeparatedList(descriptions));
            }
            break;
        }
        return QVariant();
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
            if (role == Roles::KeySequence) {
                m_internalConflictDetector.remove(m_shortcuts[index.row()].keySequence, index.row());
                const auto sequence = value.value<QKeySequence>();
                m_shortcuts[index.row()].keySequence = sequence;
                m_globalConflicts[index.row()] = !KGlobalAccel::isGlobalShortcutAvailable(sequence);
                m_standardConflicts[index.row()] = KStandardShortcut::find(sequence);
                m_internalConflictDetector.insert(sequence, index.row());
                Q_EMIT dataChanged(index, index, {Roles::KeySequence, Roles::GlobalConflict, Roles::StandardConflict, Roles::ConflictText});
                Q_EMIT dataChanged(this->index(0, 0), this->index(0, m_shortcuts.size() - 1), {Roles::InternalConflict});
                Q_EMIT hasGlobalConflictChanged();
                Q_EMIT hasInternalConflictChanged();
                return true;
            }
        }
        return false;
    }
    bool hasGlobalConflict()
    {
        return std::ranges::any_of(m_globalConflicts, std::identity());
    }
    bool hasInternalConflict()
    {
        const auto sequences = m_internalConflictDetector.uniqueKeys();
        if (sequences.contains(QKeySequence())) {
            // Allow multiple shortcuts having no sequence
            return (sequences.size() - 1 + m_internalConflictDetector.count(QKeySequence())) < m_shortcuts.size();
        }
        return sequences.size() < m_shortcuts.size();
    }
Q_SIGNALS:
    void hasGlobalConflictChanged();
    void hasInternalConflictChanged();

private:
    QList<ShortcutInfo> m_shortcuts;
    QList<bool> m_globalConflicts;
    QList<KStandardShortcut::StandardShortcut> m_standardConflicts;
    QMultiHash<QKeySequence, int> m_internalConflictDetector;
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
    return 2;
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
        return PortalResponse::OtherError;
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
    return PortalResponse::Success;
}

uint GlobalShortcutsPortal::ListShortcuts(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle, QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "ListShortcuts called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();

    auto session = Session::getSession<GlobalShortcutsSession>(session_handle.path());
    if (!session) {
        return PortalResponse::OtherError;
    }
    results = {
        {u"shortcuts"_s, session->shortcutDescriptionsVariant()},
    };
    return PortalResponse::Success;
}

void GlobalShortcutsPortal::BindShortcuts(const QDBusObjectPath &handle,
                                          const QDBusObjectPath &session_handle,
                                          const Shortcuts &shortcuts,
                                          const QString &parent_window,
                                          const QVariantMap &options,
                                          const QDBusMessage &message,
                                          uint &replyResponse,
                                          QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "BindShortcuts called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    shortcuts: " << shortcuts;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    options: " << options;

    auto session = Session::getSession<GlobalShortcutsSession>(session_handle.path());
    if (!session) {
        replyResponse = PortalResponse::OtherError;
        return;
    }

    auto previousShortcuts = session->shortcutDescriptions();
    auto currentShortcuts = shortcuts;
    std::ranges::sort(currentShortcuts, {}, &Shortcut::first);
    std::ranges::sort(previousShortcuts, {}, &Shortcut::first);

    Shortcuts newShortcuts;
    std::ranges::set_difference(currentShortcuts, previousShortcuts, std::back_inserter(newShortcuts), {}, &Shortcut::first, &Shortcut::first);
    Shortcuts returningShortcuts;
    std::ranges::set_intersection(currentShortcuts, previousShortcuts, std::back_inserter(returningShortcuts), {}, &Shortcut::first, &Shortcut::first);

    auto toShortcutInfo = [](const Shortcut &shortcut) {
        const QKeySequence preferredKeySequence = XdgShortcut::parse(shortcut.second.value(u"preferred_trigger"_s).toString()).value_or(QKeySequence());
        return ShortcutInfo{.id = shortcut.first,
                            .description = shortcut.second.value(u"description"_s).toString(),
                            .keySequence = preferredKeySequence,
                            .preferredKeySequence = preferredKeySequence};
    };

    QList<ShortcutInfo> newShortcutInfos;
    newShortcutInfos.reserve(newShortcuts.size());
    std::ranges::transform(newShortcuts, std::back_inserter(newShortcutInfos), toShortcutInfo);

    QList<ShortcutInfo> returningShortcutInfos;
    returningShortcutInfos.reserve(returningShortcuts.size());
    std::ranges::transform(returningShortcuts, std::back_inserter(returningShortcutInfos), toShortcutInfo);

    if (newShortcutInfos.empty()) {
        replyResponse = PortalResponse::Success;
        replyResults = {{u"shortcuts"_s, session->shortcutDescriptionsVariant()}};
        return;
    }

    auto dialog = new QuickDialog();
    Request::makeClosableDialogRequestWithSession(handle, dialog, session);
    auto model = new ShortcutsModel(newShortcutInfos, dialog);
    dialog->create(u"GlobalShortcutsDialog"_s,
                   {{u"app"_s, Utils::applicationName(session->appId())},
                    {u"returningShortcuts"_s, QVariant::fromValue(returningShortcutInfos)},
                    {u"newShortcuts"_s, QVariant::fromValue(model)},
                    {u"component"_s, session->componentName()}});
    Utils::setParentWindow(dialog->windowHandle(), parent_window);
    delayReply(message, dialog, session, [model, returningShortcutInfos, session](DialogResult result) -> QVariantList {
        // The dialog asks the user if they want to add the new bindings, if denied we still allow
        // binding the returning shortcuts if there are any.
        auto newShortcutInfos = model->shortcuts();
        if (result == DialogResult::Rejected) {
            newShortcutInfos.clear();
            if (returningShortcutInfos.empty()) {
                session->setActions({});
                return QVariantList{PortalResponse::Cancelled, QVariantMap{}};
            }
        }
        QList<ShortcutInfo> currentShortcutInfos;
        currentShortcutInfos.reserve(newShortcutInfos.size() + returningShortcutInfos.size());
        std::ranges::set_union(newShortcutInfos, returningShortcutInfos, std::back_inserter(currentShortcutInfos), {}, &ShortcutInfo::id, &ShortcutInfo::id);
        session->setActions(currentShortcutInfos);
        return {PortalResponse::Success, QVariantMap{{u"shortcuts"_s, session->shortcutDescriptionsVariant()}}};
    });
}

void GlobalShortcutsPortal::ConfigureShortcuts(const QDBusObjectPath &session_handle, const QString &parent_window, const QVariantMap &options)
{
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "ConfigureShortcuts called with parameters:";
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeGlobalShortcuts) << "    options: " << options;

    auto session = Session::getSession<GlobalShortcutsSession>(session_handle.path());
    if (!session) {
        return;
    }
    auto job = new KIO::OpenUrlJob(QUrl(u"systemsettings://kcm_keys/"_s + session->componentName()));
    job->setStartupId(options.value(u"activation_token"_s).value<QByteArray>());
    job->start();
}

#include "globalshortcuts.moc"
