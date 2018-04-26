/*
 * Copyright © 2018 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "screenshot.h"
#include "screenshotdialog.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QPointer>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenshot, "xdp-kde-screenshot")

ScreenshotPortal::ScreenshotPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

ScreenshotPortal::~ScreenshotPortal()
{
}

uint ScreenshotPortal::Screenshot(const QDBusObjectPath &handle,
                                  const QString &app_id,
                                  const QString &parent_window,
                                  const QVariantMap &options,
                                  QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdeScreenshot) << "Screenshot called with parameters:";
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeScreenshot) << "    options: " << options;

    QPointer<ScreenshotDialog> screenshotDialog = new ScreenshotDialog;

    const bool modal = options.value(QLatin1String("modal"), false).toBool();
    screenshotDialog->setModal(modal);

    const bool interactive = options.value(QLatin1String("interactive"), false).toBool();
    if (!interactive) {
        screenshotDialog->takeScreenshot();
    }

    QImage screenshot = screenshotDialog->exec() ? screenshotDialog->image() : QImage();

    if (screenshotDialog) {
        screenshotDialog->deleteLater();
    }

    if (screenshot.isNull()) {
        return 1;
    }

    const QString filename = QStringLiteral("%1/Screenshot_%2.png").arg(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
                                                             .arg(QDateTime::currentDateTime().toString(QLatin1String("yyyyMMdd_hhmmss")));

    if (!screenshot.save(filename, "PNG")) {
        return 1;
    }

    const QString resultFileName = QStringLiteral("file://") + filename;
    results.insert(QLatin1String("uri"), resultFileName);

    return 0;
}


