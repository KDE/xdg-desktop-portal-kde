/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screenshotdialog.h"
#include "ui_screenshotdialog.h"

#include <QLoggingCategory>
#include <QPushButton>

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QFutureWatcher>
#include <QTimer>
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

ScreenshotDialog::ScreenshotDialog(QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_dialog(new Ui::ScreenshotDialog)
{
    m_dialog->setupUi(this);

    connect(m_dialog->buttonBox, &QDialogButtonBox::accepted, this, &ScreenshotDialog::accept);
    connect(m_dialog->buttonBox, &QDialogButtonBox::rejected, this, &ScreenshotDialog::reject);
    connect(m_dialog->takeScreenshotButton, &QPushButton::clicked, this, [this]() {
        QTimer::singleShot(1000 * m_dialog->delaySpinBox->value(), this, &ScreenshotDialog::takeScreenshot);
    });

    connect(m_dialog->areaComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_dialog->includeBordersCheckbox->setEnabled(index == 2);
    });

    m_dialog->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);

    setWindowTitle(i18n("Request screenshot"));
}

ScreenshotDialog::~ScreenshotDialog()
{
    delete m_dialog;
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
        m_dialog->image->setPixmap(QPixmap::fromImage(m_image).scaled(400, 320, Qt::KeepAspectRatio));
        m_dialog->buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
    });

    watcher->setFuture(future);
}

int ScreenshotDialog::mask()
{
    int mask = 0;
    if (m_dialog->includeBordersCheckbox->isChecked()) {
        mask = 1;
    }
    if (m_dialog->includeCursorCheckbox->isChecked()) {
        mask |= 1 << 1;
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
    if (m_dialog->areaComboBox->currentIndex() < 2) {
        interface.asyncCall(m_dialog->areaComboBox->currentIndex() ? QStringLiteral("screenshotScreen") : QStringLiteral("screenshotFullscreen"),
                            QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])),
                            m_dialog->includeCursorCheckbox->isChecked());
    } else {
        interface.asyncCall(QStringLiteral("interactive"), QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])), mask());
    }
    ::close(pipeFds[1]);

    return QtConcurrent::run(readImage, pipeFds[0]);
}
