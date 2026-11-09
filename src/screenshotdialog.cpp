/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screenshotdialog.h"
#include "screenshotdialog_debug.h"

#include <QPushButton>

#include <KLocalizedString>
#include <QColorSpace>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QFutureWatcher>
#include <QStandardItemModel>
#include <QWindow>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

struct MemoryMap {
    static MemoryMap *tryMap(int fileDescriptor, size_t size)
    {
        void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fileDescriptor, 0);
        if (data == MAP_FAILED) {
            return nullptr;
        }

        return new MemoryMap{
            .data = data,
            .size = size,
        };
    }

    ~MemoryMap()
    {
        munmap(data, size);
    }

    void *data;
    size_t size;
};

static QImage readImage(int fileDescriptor, const QVariantMap &metadata)
{
    const int width = metadata.value(u"width"_s).toInt();
    if (width <= 0) {
        return QImage();
    }

    const int height = metadata.value(u"height"_s).toInt();
    if (height <= 0) {
        return QImage();
    }

    const int stride = metadata.value(u"stride"_s).toInt();
    if (stride <= 0) {
        return QImage();
    }

    // We use uint because QImage::Format values are all above 0.
    const uint format = metadata.value(u"format"_s).toUInt();
    if (format <= QImage::Format_Invalid || format >= QImage::NImageFormats) {
        return QImage();
    }

    auto memoryMap = MemoryMap::tryMap(fileDescriptor, stride * height);
    if (!memoryMap) {
        return QImage();
    }

    const auto cleanupMemoryMap = [](void *data) {
        delete static_cast<MemoryMap *>(data);
    };

    QImage image(static_cast<uchar *>(memoryMap->data), width, height, QImage::Format(format), cleanupMemoryMap, memoryMap);
    image.setColorSpace(QColorSpace::SRgb);

    return image;
}

ScreenshotDialog::ScreenshotDialog(const QString &appName, QObject *parent)
    : QuickDialog(parent)
{
    QStandardItemModel *model = new QStandardItemModel(this);
    model->appendRow(new QStandardItem(i18n("Full Screen")));
    model->appendRow(new QStandardItem(i18n("Current Screen")));
    model->appendRow(new QStandardItem(i18n("Active Window")));
    create(QStringLiteral("ScreenshotDialog"),
           {
               {u"app"_s, QVariant::fromValue<QObject *>(this)},
               {u"appName"_s, Utils::applicationName(appName)},
               {u"screenshotTypesModel"_s, QVariant::fromValue<QObject *>(model)},
           });
}

QImage ScreenshotDialog::image() const
{
    return m_image;
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

        if (watcher->isCanceled()) {
            return;
        }

        m_image = watcher->result();
        m_theDialog->setProperty("screenshotImage", m_image);
    });

    watcher->setFuture(future);
}

QFuture<QImage> ScreenshotDialog::takeScreenshot()
{
    QPromise<QImage> promise;
    QFuture<QImage> future = promise.future();

    QVariantMap options;
    options.insert(QStringLiteral("native-resolution"), true); // match xdg-desktop-portal-gnome, and fixes flameshot!3164
    if (m_theDialog->property("withCursor").toBool()) {
        options.insert(QStringLiteral("include-cursor"), true);
    }
    if (m_theDialog->property("withBorders").toBool()) {
        options.insert(QStringLiteral("include-decoration"), true);
    }

    QString method;
    switch (ScreenshotType(m_theDialog->property("screenshotType").toInt())) {
    case FullScreen:
        method = QStringLiteral("CaptureWorkspace");
        break;
    case CurrentScreen:
        method = QStringLiteral("CaptureActiveScreen");
        break;
    case ActiveWindow:
        method = QStringLiteral("CaptureActiveWindow");
        break;
    }

    auto message = QDBusMessage::createMethodCall(u"org.kde.KWin.ScreenShot3"_s, u"/org/kde/KWin/ScreenShot3"_s, u"org.kde.KWin.ScreenShot3"_s, method);
    message.setArguments({options});

    promise.start();

    const QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [watcher, p = std::move(promise)]() mutable {
        watcher->deleteLater();

        const QDBusPendingReply<QDBusUnixFileDescriptor, QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qCWarning(XdgDesktopPortalKdeScreenshotDialog) << "Failed to take a screenshot:" << reply.error();

            // The promise will be canceled when its destructor runs, don't call finish().
            return;
        }

        const QDBusUnixFileDescriptor image = reply.argumentAt<0>();
        const QVariantMap metadata = reply.argumentAt<1>();
        p.addResult(readImage(image.fileDescriptor(), metadata));
        p.finish();
    });

    return future;
}

#include "moc_screenshotdialog.cpp"
