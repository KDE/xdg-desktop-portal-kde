#include "saverestore.h"
#include "session.h"
#include <qtimer.h>

class SaveRestoreSession : public Session
{
    Q_OBJECT
public:
    SaveRestoreSession(QObject *parent, const QString &appId, const QString &sessionHandle)
        : Session(parent, appId, sessionHandle)
    {
    }

    SessionType type() const override
    {
        return SessionType::SaveRestore;
    }

    bool awaitingSaveHintResponse() const;
    void setAwaitingSaveHintResponse(bool newAwaitingSaveHintResponse);

private:
    bool m_awaitingSaveHintResponse = false;
};

class SessionStateAdaptor : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.SessionState")

public:
    explicit SessionStateAdaptor(SaveRestore *parent = nullptr);

public Q_SLOTS:
    bool saveState()
    {
        m_saveRestore->saveState(message());
        return true; // reply value will be ignored, handled async above
    }

private:
    SaveRestore *m_saveRestore;
};

bool SaveRestoreSession::awaitingSaveHintResponse() const
{
    return m_awaitingSaveHintResponse;
}

void SaveRestoreSession::setAwaitingSaveHintResponse(bool newAwaitingSaveHintResponse)
{
    m_awaitingSaveHintResponse = newAwaitingSaveHintResponse;
}

SessionStateAdaptor::SessionStateAdaptor(SaveRestore *parent)
    : QObject(parent)
    , m_saveRestore(parent)
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject(QStringLiteral("/org/kde/SessionState"), this, QDBusConnection::ExportAdaptors);
}

void SaveRestore::saveState(const QDBusMessage &message)
{
    if (m_pendingSaveReply.type() != QDBusMessage::InvalidMessage) {
        qWarning() << "requesting save, whilst save action is in progress. Ignoring";
        return;
    }

    m_pendingSaveReply = message;
    message.setDelayedReply(true);

    for (auto handle : std::as_const(m_registrations)) {
        auto session = SaveRestoreSession::getSession<SaveRestoreSession>(handle);
        if (session->type() == SaveRestoreSession::SessionType::SaveRestore) {
            session->setAwaitingSaveHintResponse(true);
            Q_EMIT SaveHint(QDBusObjectPath(session->handle()));
        }
    }
    querySaveRequestComplete();
    QTimer::singleShot(std::chrono::milliseconds(1500), this, &SaveRestore::saveRequestComplete);
}

void SaveRestore::querySaveRequestComplete()
{
    // Dave someone will complain later that this should be std::any...
    bool allComplete = true;
    for (const QString &handle : m_registrations.values()) {
        auto session = SaveRestoreSession::getSession<SaveRestoreSession>(handle);
        if (session && session->awaitingSaveHintResponse()) {
            allComplete = false;
        }
    }

    if (allComplete) {
        saveRequestComplete();
    }
}

void SaveRestore::saveRequestComplete()
{
    if (m_pendingSaveReply.type() != QDBusMessage::MethodCallMessage) {
        return;
    }

    QDBusMessage message = m_pendingSaveReply.createReply();
    QDBusConnection::sessionBus().send(message);
    m_pendingSaveReply = QDBusMessage();
}

void SaveRestore::SaveHintResponse(const QDBusObjectPath &session_handle)
{
    auto session = SaveRestoreSession::getSession<SaveRestoreSession>(session_handle.path());
    if (session && session->type() == SaveRestoreSession::SessionType::SaveRestore) {
        session->setAwaitingSaveHintResponse(false);
    }
    querySaveRequestComplete();
}

QVariantMap SaveRestore::Register(const QDBusObjectPath &session_handle, const QString &app_id, const QVariantMap &options)
{
    auto *session = new SaveRestoreSession(this, app_id, session_handle.path());
    connect(session, &Session::closed, this, [this, session]() {
        m_registrations.remove(session->appId(), session->handle());
    });
    m_registrations.insert(session->appId(), session->handle());

    int runningCount = m_registrations.count(session->appId());

    // TODO, save to a config, use that to determine pristine vs launch?
    // if startup use Restore

    QVariantMap response;
    response[QLatin1String("instance-id")] = QStringLiteral("instance%1").arg(runningCount);
    response[QLatin1String("restore-reason")] = 1;
    return response;
}

#include "saverestore.moc"
