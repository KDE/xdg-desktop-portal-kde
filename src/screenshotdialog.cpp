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
#include <poll.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenshotDialog, "xdp-kde-screenshot-dialog")

static int readData(int fd, QByteArray &data)
{
    char buffer[4096];
    pollfd pfds[1];
    pfds[0].fd = fd;
    pfds[0].events = POLLIN;

    while (true) {
        // give user 30 sec to click a window, afterwards considered as error
        const int ready = poll(pfds, 1, 30000);
        if (ready < 0) {
            if (errno != EINTR) {
                qWarning() << "poll() failed:" << strerror(errno);
                return -1;
            }
        } else if (ready == 0) {
            qDebug() << "failed to read screenshot: timeout";
            return -1;
        } else if (pfds[0].revents & POLLIN) {
            const int n = QT_READ(fd, buffer, sizeof(buffer));

            if (n < 0) {
                qWarning() << "read() failed:" << strerror(errno);
                return -1;
            } else if (n == 0) {
                return 0;
            } else if (n > 0) {
                data.append(buffer, n);
            }
        } else if (pfds[0].revents & POLLHUP) {
            return 0;
        } else {
            qWarning() << "failed to read screenshot: pipe is broken";
            return -1;
        }
    }

    Q_UNREACHABLE();
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

QFuture<QImage> ScreenshotDialog::takeScreenshot()
{
    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC | O_NONBLOCK) != 0) {
        Q_EMIT failed();
        return {};
    }

    const bool withBorders = m_theDialog->property("withBorders").toBool();
    const bool withCursor = m_theDialog->property("withCursor").toBool();

    QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));
    auto types = ScreenshotType(m_theDialog->property("screenshotType").toInt());
    if (types == ActiveWindow) {
        int mask = 0;
        if (withBorders) {
            mask = Borders;
        }
        if (withCursor) {
            mask |= Cursor;
        }
        interface.asyncCall(QStringLiteral("interactive"), QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])), mask);
    } else {
        interface.asyncCall(types ? QStringLiteral("screenshotScreen") : QStringLiteral("screenshotFullscreen"),
                            QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])),
                            withCursor);
    }
    ::close(pipeFds[1]);

    return QtConcurrent::run(readImage, pipeFds[0]);
}
