#include "location.h"
#include "location_debug.h"

#include "dbushelpers.h"
#include "quickdialog.h"
#include "request.h"
#include "utils.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QWindow>

using namespace Qt::StringLiterals;

static QString geoClue2Service()
{
    return QStringLiteral("org.freedesktop.GeoClue2");
}

static QString geoClue2Path()
{
    return QStringLiteral("/org/freedesktop/GeoClue2/Manager");
}

static QString geoClue2Interface()
{
    return QStringLiteral("org.freedesktop.GeoClue2.Manager");
}

LocationPortal::LocationPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

uint LocationPortal::CreateSession(const QDBusObjectPath &handle,
                                   const QDBusObjectPath &session_handle,
                                   const QString &app_id,
                                   const QVariantMap &options,
                                   QVariantMap &results)
{
    Q_UNUSED(results);
    qCDebug(XdgDesktopPortalKdeLocation) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeLocation) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeLocation) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeLocation) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeLocation) << "    options: " << options;

    Session *session = Session::createSession(this, Session::Location, app_id, session_handle.path());

    if (!session) {
        return PortalResponse::OtherError;
    }

    connect(session, &Session::closed, [session] {
        auto locationSession = qobject_cast<LocationSession *>(session);
        auto msg = QDBusMessage::createMethodCall(geoClue2Service(), geoClue2Path(), geoClue2Interface(), QStringLiteral("disconnect"));
        QDBusConnection::sessionBus().send(msg);
    });

    return PortalResponse::Success;
}

void LocationPortal::Start(const QDBusObjectPath &handle,
                           const QDBusObjectPath &session_handle,
                           const QString &app_id,
                           const QString &parent_window,
                           const QVariantMap &options,
                           const QDBusMessage &message,
                           uint &replyResponse,
                           QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeLocation) << "Start called with parameters:";
    qCDebug(XdgDesktopPortalKdeLocation) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeLocation) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeLocation) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeLocation) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeLocation) << "    options: " << options;

    LocationSession *session = Session::getSession<LocationSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeLocation) << "Tried to call start on non-existing session " << session_handle.path();
        replyResponse = PortalResponse::OtherError;
        return;
    }
}

#include "location.moc"