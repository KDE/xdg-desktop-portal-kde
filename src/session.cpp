/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "session.h"
#include "desktopportal.h"
#include "session_debug.h"
#include "xdgshortcut.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QLoggingCategory>
#include <screencast_debug.h>

#include <KGlobalAccel>
#include <KLocalizedString>
#include <KStatusNotifierItem>

#include "kglobalaccel_component_interface.h"
#include "kglobalaccel_interface.h"
#include "remotedesktopdialog.h"
#include "utils.h"
#include <QMenu>

static QMap<QString, Session *> sessionList;

Session::Session(QObject *parent, const QString &appId, const QString &path)
    : QDBusVirtualObject(parent)
    , m_appId(appId)
    , m_path(path)
{
}

Session::~Session()
{
}

bool Session::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    Q_UNUSED(connection);

    if (message.path() != m_path) {
        return false;
    }

    /* Check to make sure we're getting properties on our interface */
    if (message.type() != QDBusMessage::MessageType::MethodCallMessage) {
        return false;
    }

    qCDebug(XdgSessionKdeSession) << message.interface();
    qCDebug(XdgSessionKdeSession) << message.member();
    qCDebug(XdgSessionKdeSession) << message.path();

    if (message.interface() == QLatin1String("org.freedesktop.impl.portal.Session")) {
        if (message.member() == QLatin1String("Close")) {
            close();
            QDBusMessage reply = message.createReply();
            return connection.send(reply);
        }
    } else if (message.interface() == QLatin1String("org.freedesktop.DBus.Properties")) {
        if (message.member() == QLatin1String("Get")) {
            if (message.arguments().count() == 2) {
                const QString interface = message.arguments().at(0).toString();
                const QString property = message.arguments().at(1).toString();

                if (interface == QLatin1String("org.freedesktop.impl.portal.Session") && property == QLatin1String("version")) {
                    QList<QVariant> arguments;
                    arguments << 1;

                    QDBusMessage reply = message.createReply();
                    reply.setArguments(arguments);
                    return connection.send(reply);
                }
            }
        }
    }

    return false;
}

QString Session::introspect(const QString &path) const
{
    QString nodes;

    if (path.startsWith(QLatin1String("/org/freedesktop/portal/desktop/session/"))) {
        nodes = QStringLiteral(
            "<interface name=\"org.freedesktop.impl.portal.Session\">"
            "    <method name=\"Close\">"
            "    </method>"
            "<signal name=\"Closed\">"
            "</signal>"
            "<property name=\"version\" type=\"u\" access=\"read\"/>"
            "</interface>");
    }

    qCDebug(XdgSessionKdeSession) << nodes;

    return nodes;
}

bool Session::close()
{
    QDBusMessage reply = QDBusMessage::createSignal(m_path, QStringLiteral("org.freedesktop.impl.portal.Session"), QStringLiteral("Closed"));
    const bool result = QDBusConnection::sessionBus().send(reply);

    Q_EMIT closed();

    sessionList.remove(m_path);
    QDBusConnection::sessionBus().unregisterObject(m_path);

    deleteLater();

    return result;
}

Session *Session::createSession(QObject *parent, SessionType type, const QString &appId, const QString &path)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    Session *session = nullptr;
    switch (type) {
    case ScreenCast:
        session = new ScreenCastSession(parent, appId, path, QStringLiteral("media-record"));
        break;
    case RemoteDesktop:
        session = new RemoteDesktopSession(parent, appId, path);
        break;
    case GlobalShortcuts:
        session = new GlobalShortcutsSession(parent, appId, path);
        break;
    }

    if (sessionBus.registerVirtualObject(path, session, QDBusConnection::VirtualObjectRegisterOption::SubPath)) {
        connect(session, &Session::closed, [session, path]() {
            sessionList.remove(path);
            QDBusConnection::sessionBus().unregisterObject(path);
            session->deleteLater();
        });
        sessionList.insert(path, session);
        return session;
    } else {
        qCDebug(XdgSessionKdeSession) << sessionBus.lastError().message();
        qCDebug(XdgSessionKdeSession) << "Failed to register session object: " << path;
        session->deleteLater();
        return nullptr;
    }
}

Session *Session::getSession(const QString &sessionHandle)
{
    return sessionList.value(sessionHandle);
}

ScreenCastSession::ScreenCastSession(QObject *parent, const QString &appId, const QString &path, const QString &iconName)
    : Session(parent, appId, path)
    , m_item(new KStatusNotifierItem(this))
{
    m_item->setStandardActionsEnabled(false);
    m_item->setIconByName(iconName);

    auto menu = new QMenu;
    auto stopAction = new QAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18nc("@action:inmenu stops screen/window sharing", "End"));
    connect(stopAction, &QAction::triggered, this, &Session::close);
    connect(m_item, &KStatusNotifierItem::activateRequested, menu, &QMenu::show);
    menu->addAction(stopAction);
    m_item->setContextMenu(menu);
}

ScreenCastSession::~ScreenCastSession()
{
}

bool ScreenCastSession::multipleSources() const
{
    return m_multipleSources;
}

ScreenCastPortal::SourceType ScreenCastSession::types() const
{
    return m_types;
}

void ScreenCastSession::setPersistMode(ScreenCastPortal::PersistMode persistMode)
{
    m_persistMode = persistMode;
}

ScreenCastPortal::CursorModes ScreenCastSession::cursorMode() const
{
    return m_cursorMode;
}

void ScreenCastSession::setOptions(const QVariantMap &options)
{
    m_multipleSources = options.value(QStringLiteral("multiple")).toBool();
    m_cursorMode = ScreenCastPortal::CursorModes(options.value(QStringLiteral("cursor_mode")).toUInt());
    m_types = ScreenCastPortal::SourceType(options.value(QStringLiteral("types")).toUInt());

    if (m_types == 0) {
        m_types = ScreenCastPortal::Monitor;
    }
}

RemoteDesktopSession::RemoteDesktopSession(QObject *parent, const QString &appId, const QString &path)
    : ScreenCastSession(parent, appId, path, QStringLiteral("krfb"))
    , m_screenSharingEnabled(false)
{
    connect(this, &RemoteDesktopSession::closed, this, [this] {
        if (m_acquired) {
            WaylandIntegration::acquireStreamingInput(false);
        }
    });
}

RemoteDesktopSession::~RemoteDesktopSession()
{
}

void RemoteDesktopSession::setOptions(const QVariantMap &options)
{
}

RemoteDesktopPortal::DeviceTypes RemoteDesktopSession::deviceTypes() const
{
    return m_deviceTypes;
}

void RemoteDesktopSession::setDeviceTypes(RemoteDesktopPortal::DeviceTypes deviceTypes)
{
    m_deviceTypes = deviceTypes;
}

bool RemoteDesktopSession::screenSharingEnabled() const
{
    return m_screenSharingEnabled;
}

void RemoteDesktopSession::setScreenSharingEnabled(bool enabled)
{
    if (m_screenSharingEnabled == enabled) {
        return;
    }

    m_screenSharingEnabled = enabled;
}

void RemoteDesktopSession::acquireStreamingInput()
{
    WaylandIntegration::acquireStreamingInput(true);
    m_acquired = true;
}

void RemoteDesktopSession::refreshDescription()
{
    m_item->setTitle(i18nc("SNI title that indicates there's a process remotely controlling the system", "Remote Desktop"));
    m_item->setToolTipTitle(m_item->title());
    setDescription(RemoteDesktopDialog::buildDescription(m_appId, deviceTypes(), screenSharingEnabled()));
}

void ScreenCastSession::setStreams(const WaylandIntegration::Streams &streams)
{
    Q_ASSERT(!streams.isEmpty());
    m_streams = streams;

    m_item->setStandardActionsEnabled(false);
    if (qobject_cast<RemoteDesktopSession *>(this)) {
        refreshDescription();
    } else {
        const bool isWindow = m_streams[0].map[QLatin1String("source_type")] == ScreenCastPortal::Window;
        m_item->setToolTipSubTitle(i18ncp("%1 number of screens, %2 the app that receives them",
                                          "Sharing contents to %2",
                                          "%1 video streams to %2",
                                          m_streams.count(),
                                          Utils::applicationName(m_appId)));
        m_item->setTitle(i18nc("SNI title that indicates there's a process seeing our windows or screens", "Screen casting"));
        if (isWindow) {
            m_item->setOverlayIconByName(QStringLiteral("window"));
        } else {
            m_item->setOverlayIconByName(QStringLiteral("monitor"));
        }
    }
    m_item->setToolTipIconByName(m_item->overlayIconName());
    m_item->setToolTipTitle(m_item->title());

    for (const auto &s : streams) {
        connect(s.stream, &ScreencastingStream::closed, this, &ScreenCastSession::streamClosed);
        connect(s.stream, &ScreencastingStream::failed, this, [this](const QString &error) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "ScreenCast session failed" << error;
            streamClosed();
        });
    }
    m_item->setStatus(KStatusNotifierItem::Active);
}

void ScreenCastSession::setDescription(const QString &description)
{
    m_item->setToolTipSubTitle(description);
}

void ScreenCastSession::streamClosed()
{
    ScreencastingStream *stream = qobject_cast<ScreencastingStream *>(sender());
    auto it = std::remove_if(m_streams.begin(), m_streams.end(), [stream](const WaylandIntegration::Stream &s) {
        return s.stream == stream;
    });
    m_streams.erase(it, m_streams.end());

    if (m_streams.isEmpty()) {
        close();
    }
}

GlobalShortcutsSession::GlobalShortcutsSession(QObject *parent, const QString &appId, const QString &path)
    : Session(parent, appId, path)
    , m_token(path.mid(path.lastIndexOf('/') + 1))
    , m_globalAccelInterface(
          new KGlobalAccelInterface(QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/kglobalaccel"), QDBusConnection::sessionBus(), this))
    , m_component(new KGlobalAccelComponentInterface(m_globalAccelInterface->service(),
                                                     m_globalAccelInterface->getComponent(componentName()).value().path(),
                                                     m_globalAccelInterface->connection(),
                                                     this))
{
    qDBusRegisterMetaType<KGlobalShortcutInfo>();
    qDBusRegisterMetaType<QList<KGlobalShortcutInfo>>();
    qDBusRegisterMetaType<QKeySequence>();
    qDBusRegisterMetaType<QList<QKeySequence>>();

    connect(m_globalAccelInterface,
            &KGlobalAccelInterface::yourShortcutsChanged,
            this,
            [this](const QStringList &actionId, const QList<QKeySequence> &newKeys) {
                Q_UNUSED(newKeys);
                if (actionId[KGlobalAccel::ComponentUnique] == componentName()) {
                    m_shortcuts[actionId[KGlobalAccel::ActionUnique]]->setShortcuts(newKeys);
                    Q_EMIT shortcutsChanged();
                }
            });
    connect(m_component,
            &KGlobalAccelComponentInterface::globalShortcutPressed,
            this,
            [this](const QString &componentUnique, const QString &actionUnique, qlonglong timestamp) {
                if (componentUnique != componentName()) {
                    return;
                }

                Q_EMIT shortcutActivated(actionUnique, timestamp);
            });
    connect(m_component,
            &KGlobalAccelComponentInterface::globalShortcutReleased,
            this,
            [this](const QString &componentUnique, const QString &actionUnique, qlonglong timestamp) {
                if (componentUnique != componentName()) {
                    return;
                }

                Q_EMIT shortcutDeactivated(actionUnique, timestamp);
            });
}

GlobalShortcutsSession::~GlobalShortcutsSession() = default;

void GlobalShortcutsSession::restoreActions(const QVariant &shortcutsVariant)
{
    auto arg = shortcutsVariant.value<QDBusArgument>();

    static const auto ShortcutsSignature = QDBusMetaType::typeToSignature(QMetaType(qMetaTypeId<Shortcuts>()));
    if (arg.currentSignature() != QLatin1String(ShortcutsSignature)) {
        qCWarning(XdgSessionKdeSession) << "Wrong global shortcuts type, should be " << ShortcutsSignature << "instead of " << arg.currentSignature();
        return;
    }

    const Shortcuts shortcuts = qdbus_cast<Shortcuts>(arg);
    const QList<KGlobalShortcutInfo> shortcutInfos = m_component->allShortcutInfos();
    QHash<QString, KGlobalShortcutInfo> shortcutInfosByName;
    shortcutInfosByName.reserve(shortcutInfos.size());
    for (const auto &shortcutInfo : shortcutInfos) {
        shortcutInfosByName[shortcutInfo.uniqueName()] = shortcutInfo;
    }

    for (const auto &shortcut : shortcuts) {
        const QString description = shortcut.second["description"].toString();
        if (description.isEmpty() || shortcut.first.isEmpty()) {
            qCWarning(XdgSessionKdeSession) << "Shortcut without name or description" << shortcut.first << "for" << componentName();
            continue;
        }

        QAction *&action = m_shortcuts[shortcut.first];
        if (!action) {
            action = new QAction(this);
        }
        action->setProperty("componentName", componentName());
        action->setProperty("componentDisplayName", componentName());
        action->setObjectName(shortcut.first);
        action->setText(description);
        const auto itShortcut = shortcutInfosByName.constFind(shortcut.first);
        if (itShortcut != shortcutInfosByName.constEnd()) {
            action->setShortcuts(itShortcut->keys());
        } else {
            const auto preferredShortcut = XdgShortcut::parse(shortcut.second["preferred_trigger"].toString());
            if (preferredShortcut) {
                action->setShortcut(preferredShortcut.value());
            }
        }
        KGlobalAccel::self()->setGlobalShortcut(action, action->shortcuts());

        shortcutInfosByName.remove(shortcut.first);
    }

    // We can forget the shortcuts that aren't around anymore
    while (!shortcutInfosByName.isEmpty()) {
        const QString shortcutName = shortcutInfosByName.begin().key();
        auto action = m_shortcuts.take(shortcutName);
        KGlobalAccel::self()->removeAllShortcuts(action);
        shortcutInfosByName.erase(shortcutInfosByName.begin());
    }

    Q_ASSERT(m_shortcuts.size() == shortcuts.size());
}

QVariant GlobalShortcutsSession::shortcutDescriptionsVariant() const
{
    QDBusArgument retVar;
    retVar << shortcutDescriptions();
    return QVariant::fromValue(retVar);
}

Shortcuts GlobalShortcutsSession::shortcutDescriptions() const
{
    Shortcuts ret;
    for (auto it = m_shortcuts.cbegin(), itEnd = m_shortcuts.cend(); it != itEnd; ++it) {
        QStringList triggers;
        triggers.reserve((*it)->shortcuts().size());
        const auto shortcuts = (*it)->shortcuts();
        for (const auto &shortcut : shortcuts) {
            triggers += shortcut.toString(QKeySequence::NativeText);
        }

        ret.append({it.key(),
                    QVariantMap{
                        {QStringLiteral("description"), (*it)->text()},
                        {QStringLiteral("trigger_description"), triggers.join(i18n(", "))},
                    }});
    }
    Q_ASSERT(ret.size() == m_shortcuts.size());
    return ret;
}
