/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screenshotdialog.h"

#include <QLoggingCategory>
#include <QPushButton>

#include <KLocalizedString>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QFutureWatcher>
#include <QStandardItemModel>
#include <QTimer>
#include <QWindow>
#include <QtConcurrentRun>
#include <qplatformdefs.h>

#include <fcntl.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenshotDialog, "xdp-kde-screenshot-dialog")

static int readData(int fd, QByteArray &data)
{
    // implementation based on QtWayland file qwaylanddataoffer.cpp
    char buf[4096];
    int retryCount = 0;
    int n;
    while (true) {
        n = QT_READ(fd, buf, sizeof buf);
        // give user 30 sec to click a window, afterwards considered as error
        if (n == -1 && (errno == EAGAIN) && ++retryCount < 30000) {
            usleep(1000);
        } else {
            break;
        }
    }
    if (n > 0) {
        data.append(buf, n);
        n = readData(fd, data);
    }
    return n;
}

static QImage readImage(int pipeFd)
{
    QByteArray content;
    if (readData(pipeFd, content) != 0) {
        close(pipeFd);
        return QImage();
    }
    close(pipeFd);
    QDataStream ds(content);
    QImage image;
    ds >> image;
    return image;
}

ScreenshotDialog::ScreenshotDialog(QObject *parent)
    : QuickDialog(parent)
{
    QStandardItemModel *model = new QStandardItemModel(this);
    model->appendRow(new QStandardItem(i18n("Full Screen")));
    model->appendRow(new QStandardItem(i18n("Current Screen")));
    model->appendRow(new QStandardItem(i18n("Active Window")));
    create("qrc:/ScreenshotDialog.qml",
           {
               {"app", QVariant::fromValue<QObject *>(this)},
               {"screenshotTypesModel", QVariant::fromValue<QObject *>(model)},
           });
}

QImage ScreenshotDialog::image() const
{
    return m_image;
}

void ScreenshotDialog::takeScreenshotNonInteractive()
{
    QFuture<QImage> future = takeScreenshot();
    if (!future.isStarted()) {
        return;
    }

    future.waitForFinished();
    m_image = future.result();
}

void ScreenshotDialog::takeScreenshotInteractive()
{
    const QFuture<QImage> future = takeScreenshot();
    if (!future.isStarted()) {
        return;
    }

    QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>(this);
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished, this, [watcher, this] {
        watcher->deleteLater();
        m_image = watcher->result();
        m_theDialog->setProperty("screenshotImage", m_image);
    });

    watcher->setFuture(future);
}

int ScreenshotDialog::mask()
{
    int mask = 0;
    if (m_theDialog->property("withBorders").toBool()) {
        mask = Borders;
    }
    if (m_theDialog->property("withCursor").toBool()) {
        mask |= Cursor;
    }
    return mask;
}

QFuture<QImage> ScreenshotDialog::takeScreenshot()
{
    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC | O_NONBLOCK) != 0) {
        Q_EMIT failed();
        return {};
    }

    QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));
    auto types = ScreenshotType(m_theDialog->property("screenshotType").toInt());
    if (types == ActiveWindow) {
        interface.asyncCall(QStringLiteral("interactive"), QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])), mask());
    } else {
        interface.asyncCall(types ? QStringLiteral("screenshotScreen") : QStringLiteral("screenshotFullscreen"),
                            QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])),
                            mask());
    }
    ::close(pipeFds[1]);

    return QtConcurrent::run(readImage, pipeFds[0]);
}
