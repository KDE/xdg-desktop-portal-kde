#include "saverestore.h"
#include "session.h"

class SaveRestoreSession : public Session
{
public:
    SaveRestoreSession(QObject *parent, const QString &appId, const QString &sessionHandle)
        : Session(parent, appId, sessionHandle)
    {
    }

    SessionType type() const override
    {
        return SessionType::SaveRestore;
    }
};

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
