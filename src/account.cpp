/*
 * SPDX-FileCopyrightText: 2020 Red Hat Inc
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2020 Jan Grulich <jgrulich@redhat.com>
 */

#include "account.h"
#include "account_debug.h"
#include "dbushelpers.h"
#include "userinfodialog.h"
#include "utils.h"

#include <QDBusConnection>
#include <QDate>
#include <QDateTime>
#include <QVector>
#include <algorithm>

#include <KConfig>
#include <KConfigGroup>

AccountPortal::AccountPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

void AccountPortal::GetUserInformation(const QDBusObjectPath &handle,
                                       const QString &app_id,
                                       const QString &parent_window,
                                       const QVariantMap &options,
                                       const QDBusMessage &message,
                                       [[maybe_unused]] uint &replyResponse,
                                       [[maybe_unused]] QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeAccount) << "GetUserInformation called with parameters:";
    qCDebug(XdgDesktopPortalKdeAccount) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeAccount) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeAccount) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeAccount) << "    options: " << options;

    QString reason;

    if (options.contains(QStringLiteral("reason"))) {
        reason = options.value(QStringLiteral("reason")).toString();
    }

    UserInfoDialog *userInfoDialog = new UserInfoDialog(reason, app_id);
    Utils::setParentWindow(userInfoDialog->windowHandle(), parent_window);

    delayReply(message, userInfoDialog, this, [message, userInfoDialog](DialogResult result) {
        QVariantMap results;
        if (result == DialogResult::Accepted) {
            results.insert(QStringLiteral("id"), userInfoDialog->id());
            results.insert(QStringLiteral("name"), userInfoDialog->name());
            const QString image = userInfoDialog->image();
            results.insert(QStringLiteral("image"), image.isEmpty() ? QStringLiteral("file://") : image);
        }
        return QVariantList{PortalResponse::fromDialogResult(result), results};
    });
}

void AccountPortal::GetParentalControls(const QDBusObjectPath &handle,
                                        const QString &app_id,
                                        const QString &parent_window,
                                        uint gate1,
                                        uint gate2,
                                        uint gate3,
                                        const QVariantMap &options,
                                        const QDBusMessage &message,
                                        uint &replyResponse,
                                        QVariantMap &replyResults)
{
    Q_UNUSED(message)
    Q_UNUSED(options)

    // validate input  - maybe this should be upstream? Or both?
    // should it allow duplicates?
    if (!(gate1 < gate2 && gate2 < gate3)) {
        qCDebug(XdgDesktopPortalKdeAccount) << "Invalid gates provided";
        replyResponse = PortalResponse::OtherError;
        return;
    }

    qCDebug(XdgDesktopPortalKdeAccount) << "GetParentalControls called with parameters:";
    qCDebug(XdgDesktopPortalKdeAccount) << "    handle:" << handle.path();
    qCDebug(XdgDesktopPortalKdeAccount) << "    parent_window:" << parent_window;
    qCDebug(XdgDesktopPortalKdeAccount) << "    app_id:" << app_id;
    qCDebug(XdgDesktopPortalKdeAccount) << "    gates:" << gate1 << gate2 << gate3;

    // Obviously I don't expect it to be stored here
    // TODO!
    KConfig kdeglobals(QStringLiteral("kdeglobals"));
    KConfigGroup general(&kdeglobals, QStringLiteral("General"));

    qlonglong dobTimestamp = general.readEntry(QStringLiteral("DateOfBirth"), 0);
    int age = -1;

    if (dobTimestamp > 0) {
        const QDate birthDate = QDateTime::fromSecsSinceEpoch(dobTimestamp).date();
        const QDate today = QDate::currentDate();
        age = today.year() - birthDate.year();
        const QDate birthdayThisYear = birthDate.addYears(age);
        if (today < birthdayThisYear) {
            age -= 1;
        }
    }

    int low = -1;
    int high = -1;

    if (age < 0) {
        qCDebug(XdgDesktopPortalKdeAccount) << "Unable to determine age, aborting";
        replyResponse = PortalResponse::OtherError;
        return;
    } else {
        QVector<uint> gates = {gate1, gate2, gate3};

        // Compute low (highest gate <= age)
        for (int gate : std::as_const(gates)) {
            if (gate <= age) {
                low = gate;
            }
        }

        // Compute high (lowest gate > age)
        for (int gate : std::as_const(gates)) {
            if (gate > age) {
                high = gate;
                break;
            }
        }
        replyResponse = PortalResponse::Success;
        replyResults.insert(QStringLiteral("low"), low);
        replyResults.insert(QStringLiteral("high"), high);
    }
}

#include "moc_account.cpp"
