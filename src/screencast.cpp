/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "screencast.h"
#include "dbushelpers.h"
#include "notificationinhibition.h"
#include "request.h"
#include "restoredata.h"
#include "screencast_debug.h"
#include "screenchooserdialog.h"
#include "session.h"
#include "utils.h"
#include "waylandintegration.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDataStream>
#include <QGuiApplication>
#include <QIODevice>

#include <ranges>

using namespace Qt::StringLiterals;

struct WindowRestoreInfo {
    QString appId;
    QString title;
};

QDataStream &operator<<(QDataStream &out, const WindowRestoreInfo &info)
{
    return out << info.appId << info.title;
}
QDataStream &operator>>(QDataStream &in, WindowRestoreInfo &info)
{
    return in >> info.appId >> info.title;
}

Q_DECLARE_METATYPE(WindowRestoreInfo)

int levenshteinDistance(const QString &a, const QString &b)
{
    if (a == b) {
        return 0;
    }

    auto v0 = std::make_unique_for_overwrite<int[]>(b.size() + 1);
    auto v1 = std::make_unique_for_overwrite<int[]>(b.size() + 1);
    std::iota(v0.get(), v0.get() + b.size(), 0);

    for (int i = 0; i < a.size(); ++i) {
        v1[0] = i + 1;
        for (int j = 0; j < b.size(); ++j) {
            const int delCost = v0[j + 1] + 1;
            const int insertCost = v1[j] + 1;
            const int subsCost = a.at(i) == b.at(j) ? v0[j] : v0[j] + 1;
            v1[j + 1] = std::min({delCost, insertCost, subsCost});
        }
        std::swap(v0, v1);
    }
    return v0[b.size()];
}

QList<KWayland::Client::PlasmaWindow *> tryMatchWindows(const QList<WindowRestoreInfo> toRestore)
{
    QList<KWayland::Client::PlasmaWindow *> matches;

    const auto windows = WaylandIntegration::plasmaWindowManagement()->windows();
    for (const auto &restoreInfo : toRestore) {
        int bestDistance = INT_MAX;
        KWayland::Client::PlasmaWindow *bestMatch = nullptr;
        for (const auto window : windows) {
            if (window->appId() != restoreInfo.appId) {
                continue;
            }
            if (const int distance = levenshteinDistance(window->title(), restoreInfo.title); distance == 0) {
                matches.push_back(window);
                break;
            } else if (distance < bestDistance) {
                bestDistance = distance;
                bestMatch = window;
            }
        }
        // Arbitrary metric to make sure the best match is at least similar
        if (bestMatch && bestDistance < restoreInfo.title.size() / 2) {
            matches.push_back(bestMatch);
        }
    }
    return matches;
}

ScreenCastPortal::ScreenCastPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<RestoreData>();
    qRegisterMetaType<QList<WindowRestoreInfo>>();
}

ScreenCastPortal::~ScreenCastPortal()
{
}

bool inhibitionsEnabled()
{
    if (!WaylandIntegration::isStreamingAvailable()) {
        return false;
    }

    auto cfg = KSharedConfig::openConfig(QStringLiteral("plasmanotifyrc"));

    KConfigGroup grp(cfg, u"DoNotDisturb"_s);

    return grp.readEntry("WhenScreenSharing", true);
}

uint ScreenCastPortal::CreateSession(const QDBusObjectPath &handle,
                                     const QDBusObjectPath &session_handle,
                                     const QString &app_id,
                                     const QVariantMap &options,
                                     QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeScreenCast) << "CreateSession called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    Session *session = new ScreenCastSession(this, app_id, session_handle.path(), QStringLiteral("media-record"));

    if (!session->isValid()) {
        return PortalResponse::OtherError;
    }

    if (!WaylandIntegration::isStreamingAvailable()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "zkde_screencast_unstable_v1 does not seem to be available";
        return PortalResponse::OtherError;
    }

    connect(session, &Session::closed, [session] {
        auto screencastSession = qobject_cast<ScreenCastSession *>(session);
        const auto streams = screencastSession->streams();
        for (const WaylandIntegration::Stream &stream : streams) {
            WaylandIntegration::stopStreaming(stream.nodeId);
        }
    });
    return PortalResponse::Success;
}

uint ScreenCastPortal::SelectSources(const QDBusObjectPath &handle,
                                     const QDBusObjectPath &session_handle,
                                     const QString &app_id,
                                     const QVariantMap &options,
                                     QVariantMap &results)
{
    Q_UNUSED(results)

    qCDebug(XdgDesktopPortalKdeScreenCast) << "SelectSource called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    ScreenCastSession *session = Session::getSession<ScreenCastSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to select sources on non-existing session " << session_handle.path();
        return PortalResponse::OtherError;
    }

    session->setOptions(options);
    // Might be also a RemoteDesktopSession
    if (session->type() == Session::RemoteDesktop) {
        RemoteDesktopSession *remoteDesktopSession = qobject_cast<RemoteDesktopSession *>(session);
        if (remoteDesktopSession) {
            remoteDesktopSession->setScreenSharingEnabled(true);
        }
    } else {
        session->setPersistMode(ScreenCastPortal::PersistMode(options.value(QStringLiteral("persist_mode")).toUInt()));
        session->setRestoreData(options.value(QStringLiteral("restore_data")));
    }

    return PortalResponse::Success;
}

std::pair<PortalResponse::Response, QVariantMap> continueStartAfterDialog(ScreenCastSession *session,
                                                                          const QList<Output> &selectedOutputs,
                                                                          const QRect &selectedRegion,
                                                                          QList<KWayland::Client::PlasmaWindow *> selectedWindows,
                                                                          bool allowRestore)
{
    QVariantList outputs;
    QList<WindowRestoreInfo> windows;
    WaylandIntegration::Streams streams;
    QPointer<ScreenCastSession> guardedSession(session);
    Screencasting::CursorMode cursorMode = Screencasting::CursorMode(session->cursorMode());
    for (const auto &output : std::as_const(selectedOutputs)) {
        WaylandIntegration::Stream stream;
        switch (output.outputType()) {
        case Output::Region:
            stream = WaylandIntegration::startStreamingRegion(selectedRegion, cursorMode);
            break;
        case Output::Workspace:
            stream = WaylandIntegration::startStreamingWorkspace(cursorMode);
            break;
        case Output::Virtual: {
            const QString outputName = session->appId().isEmpty()
                ? i18n("Virtual Output")
                : i18nc("%1 is the application name", "Virtual Output (shared with %1)", Utils::applicationName(session->appId()));
            stream = WaylandIntegration::startStreamingVirtual(OutputsModel::virtualScreenIdForApp(session->appId()), outputName, {1920, 1080}, cursorMode);
            break;
        }
        default:
            stream = WaylandIntegration::startStreamingOutput(output.screen(), cursorMode);
            break;
        }

        if (!stream.isValid()) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "Invalid screen!" << output.outputType() << output.uniqueId();
            return {PortalResponse::OtherError, {}};
        }

        if (allowRestore) {
            outputs += output.uniqueId();
        }
        streams << stream;
    }
    for (const auto win : std::as_const(selectedWindows)) {
        WaylandIntegration::Stream stream = WaylandIntegration::startStreamingWindow(win, cursorMode);
        if (!stream.isValid()) {
            qCWarning(XdgDesktopPortalKdeScreenCast) << "Invalid window!" << win;
            return {PortalResponse::OtherError, {}};
        }

        if (allowRestore) {
            windows += WindowRestoreInfo{.appId = win->appId(), .title = win->title()};
        }
        streams << stream;
    }

    if (streams.isEmpty()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Pipewire stream is not ready to be streamed";
        return {PortalResponse::OtherError, {}};
    }

    if (!guardedSession) {
        return {PortalResponse::OtherError, {}};
    }

    session->setStreams(streams);
    QVariantMap results;
    results.insert(QStringLiteral("streams"), QVariant::fromValue<WaylandIntegration::Streams>(streams));
    if (allowRestore) {
        results.insert(u"persist_mode"_s, quint32(session->persistMode()));
        if (session->persistMode() != ScreenCastPortal::NoPersist) {
            const RestoreData restoreData = {u"KDE"_s,
                                             RestoreData::currentRestoreDataVersion(),
                                             QVariantMap{
                                                 {u"outputs"_s, outputs},
                                                 {u"windows"_s, QVariant::fromValue(windows)},
                                                 {u"region"_s, selectedRegion},
                                             }};
            results.insert(u"restore_data"_s, QVariant::fromValue<RestoreData>(restoreData));
        }
    }

    if (inhibitionsEnabled()) {
        new NotificationInhibition(session->appId(), i18nc("Do not disturb mode is enabled because...", "Screen sharing in progress"), session);
    }
    qCDebug(XdgDesktopPortalKdeScreenCast) << "Screencast started successfully";
    return {PortalResponse::Success, results};
}

void ScreenCastPortal::Start(const QDBusObjectPath &handle,
                             const QDBusObjectPath &session_handle,
                             const QString &app_id,
                             const QString &parent_window,
                             const QVariantMap &options,
                             const QDBusMessage &message,
                             uint &replyResponse,
                             QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeScreenCast) << "Start called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    session_handle: " << session_handle.path();
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeScreenCast) << "    options: " << options;

    QPointer<ScreenCastSession> session = Session::getSession<ScreenCastSession>(session_handle.path());

    if (!session) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Tried to call start on non-existing session " << session_handle.path();
        replyResponse = PortalResponse::OtherError;
        return;
    }

    if (QGuiApplication::screens().isEmpty()) {
        qCWarning(XdgDesktopPortalKdeScreenCast) << "Failed to show dialog as there is no screen to select";
        replyResponse = PortalResponse::OtherError;
        return;
    }

    const PersistMode persist = session->persistMode();
    bool valid = false;
    QList<Output> selectedOutputs;
    QList<KWayland::Client::PlasmaWindow *> selectedWindows;
    QRect selectedRegion;
    if (persist != NoPersist && session->restoreData().isValid()) {
        const RestoreData restoreData = qdbus_cast<RestoreData>(session->restoreData().value<QDBusArgument>());
        if (restoreData.session == QLatin1String("KDE") && restoreData.version == RestoreData::currentRestoreDataVersion()) {
            const QVariantMap restoreDataPayload = restoreData.payload;
            const QVariantList restoreOutputs = restoreDataPayload[QStringLiteral("outputs")].toList();
            if (!restoreOutputs.isEmpty()) {
                OutputsModel model(OutputsModel::WorkspaceIncluded | OutputsModel::RegionIncluded | OutputsModel::VirtualIncluded, this);
                for (const auto &outputUniqueId : restoreOutputs) {
                    for (int i = 0, c = model.rowCount(); i < c; ++i) {
                        const Output &iOutput = model.outputAt(i);
                        if (iOutput.uniqueId() == outputUniqueId) {
                            selectedOutputs << iOutput;
                        }
                    }
                }
                valid = selectedOutputs.count() == restoreOutputs.count();
            }

            const QRect restoreRegion = restoreDataPayload[QStringLiteral("region")].value<QRect>();
            if (restoreRegion.isValid()) {
                selectedRegion = restoreRegion;
                const auto screens = QGuiApplication::screens();
                QRegion fullWorkspace;
                for (const auto screen : screens) {
                    fullWorkspace += screen->geometry();
                }
                valid = fullWorkspace.contains(selectedRegion);
            }

            const auto restoreWindows = restoreDataPayload[QStringLiteral("windows")].value<QList<WindowRestoreInfo>>();
            if (!restoreWindows.isEmpty()) {
                selectedWindows = tryMatchWindows(restoreWindows);
                valid = selectedWindows.count() == restoreWindows.count();
            }
        }
    }

    if (valid) {
        std::tie(replyResponse, replyResults) = continueStartAfterDialog(session, selectedOutputs, selectedRegion, selectedWindows, true);
        return;
    }

    auto screenDialog = new ScreenChooserDialog(app_id, session->multipleSources(), SourceTypes(session->types()));
    Utils::setParentWindow(screenDialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequestWithSession(handle, screenDialog, session);
    delayReply(message, screenDialog, this, [screenDialog, session](DialogResult result) -> QVariantList {
        if (result == DialogResult::Rejected) {
            return {PortalResponse::fromDialogResult(result), QVariantMap{}};
        }
        auto [response, results] = continueStartAfterDialog(session,
                                                            screenDialog->selectedOutputs(),
                                                            screenDialog->selectedRegion(),
                                                            screenDialog->selectedWindows(),
                                                            screenDialog->allowRestore());
        return {response, results};
    });
}
