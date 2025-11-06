/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "screenshot.h"
#include "dbushelpers.h"
#include "request.h"
#include "screenshot_debug.h"
#include "screenshotdialog.h"
#include "utils.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDateTime>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>
#include <QWindow>

// Keep in sync with qflatpakcolordialog from Qt flatpak platform theme
Q_DECLARE_METATYPE(ScreenshotPortal::ColorRGB)

QDBusArgument &operator<<(QDBusArgument &arg, const ScreenshotPortal::ColorRGB &color)
{
    arg.beginStructure();
    arg << color.red << color.green << color.blue;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, ScreenshotPortal::ColorRGB &color)
{
    double red, green, blue;
    arg.beginStructure();
    arg >> red >> green >> blue;
    color.red = red;
    color.green = green;
    color.blue = blue;
    arg.endStructure();

    return arg;
}

QDBusArgument &operator<<(QDBusArgument &argument, const QColor &color)
{
    argument.beginStructure();
    argument << color.rgba();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QColor &color)
{
    argument.beginStructure();
    QRgb rgba;
    argument >> rgba;
    argument.endStructure();
    color = QColor::fromRgba(rgba);
    return argument;
}

ScreenshotPortal::ScreenshotPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<QColor>();
    qDBusRegisterMetaType<ColorRGB>();
}

ScreenshotPortal::~ScreenshotPortal()
{
}

void ScreenshotPortal::Screenshot(const QDBusObjectPath &handle,
                                  const QString &app_id,
                                  const QString &parent_window,
                                  const QVariantMap &options,
                                  const QDBusMessage &message,
                                  uint &replyResponse,
                                  QVariantMap &replyResults)
{
    qCDebug(XdgDesktopPortalKdeScreenshot) << "Screenshot called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    options: " << options;

    QPointer<ScreenshotDialog> screenshotDialog = new ScreenshotDialog;
    Utils::setParentWindow(screenshotDialog->windowHandle(), parent_window);
    Request::makeClosableDialogRequest(handle, screenshotDialog.data());

    const bool modal = options.value(QStringLiteral("modal"), true).toBool();
    screenshotDialog->windowHandle()->setModality(modal ? Qt::WindowModal : Qt::NonModal);

    const bool interactive = options.value(QStringLiteral("interactive"), false).toBool();

    auto createReply = [](const QImage &screenshot) -> std::pair<PortalResponse::Response, QVariantMap> {
        if (screenshot.isNull()) {
            return {PortalResponse::OtherError, {}};
        }
        const QString filename = QStringLiteral("%1/Screenshot_%2.png") //
                                     .arg(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                          QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
        if (!screenshot.save(filename, "PNG")) {
            return {PortalResponse::OtherError, {}};
        }
        return {PortalResponse::Success, {{QStringLiteral("uri"), QUrl::fromLocalFile(filename).toString(QUrl::FullyEncoded)}}};
    };

    if (!interactive) {
        qDebug() << "take non interactive";
        screenshotDialog->takeScreenshotNonInteractive();
        std::tie(replyResponse, replyResults) = createReply(screenshotDialog->image());
        screenshotDialog->deleteLater();
        return;
    }

    delayReply(message, screenshotDialog.get(), this, [screenshotDialog, createReply](DialogResult dialogResult) {
        auto response = PortalResponse::fromDialogResult(dialogResult);
        QVariantMap results;
        if (dialogResult == DialogResult::Accepted) {
            std::tie(response, results) = createReply(screenshotDialog->image());
        }
        return QVariantList{response, results};
    });
}

uint ScreenshotPortal::PickColor(const QDBusObjectPath &handle,
                                 const QString &app_id,
                                 const QString &parent_window,
                                 const QVariantMap &options,
                                 QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeScreenshot) << "PickColor called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    options: " << options;

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                      QStringLiteral("/ColorPicker"),
                                                      QStringLiteral("org.kde.kwin.ColorPicker"),
                                                      QStringLiteral("pick"));
    QDBusReply<QColor> reply = QDBusConnection::sessionBus().call(msg);
    if (reply.isValid() && !reply.error().isValid()) {
        QColor selectedColor = reply.value();
        ColorRGB color;
        color.red = selectedColor.redF();
        color.green = selectedColor.greenF();
        color.blue = selectedColor.blueF();

        results.insert(QStringLiteral("color"), QVariant::fromValue<ScreenshotPortal::ColorRGB>(color));
        return PortalResponse::Success;
    }

    return PortalResponse::Cancelled;
}
