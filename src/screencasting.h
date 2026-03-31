/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <qwayland-kde-screencast-v2.h>

#include <QList>
#include <QObject>
#include <QScreen>
#include <QWaylandClientExtensionTemplate>

namespace KWayland
{
namespace Client
{
class PlasmaWindow;
}
}

class ScreencastingStream : public QObject, private QtWayland::kde_screencast_stream_v2
{
    Q_OBJECT
public:
    explicit ScreencastingStream(::kde_screencast_stream_v2 *object);
    ~ScreencastingStream() override;

    quint32 nodeId;
    quint64 objectSerial;
    QRect geometry;
    QVariantMap metaData;

Q_SIGNALS:
    void created();
    void failed(const QString &error);
    void closed();

private:
    void kde_screencast_stream_v2_created(uint32_t node, uint32_t object_serial_hi, uint32_t object_serial_low) override;
    void kde_screencast_stream_v2_failed(const QString &error) override;
    void kde_screencast_stream_v2_closed() override;
    friend class Screencasting;
};

class Screencasting : public QWaylandClientExtensionTemplate<Screencasting>, public QtWayland::kde_screencast_manager_v2
{
    Q_OBJECT
public:
    explicit Screencasting();
    ~Screencasting() override;

    enum CursorMode {
        Hidden = 1,
        Embedded = 2,
        Metadata = 4,
    };
    Q_ENUM(CursorMode)

    ScreencastingStream *createOutputStream(QScreen *screen, CursorMode mode);
    ScreencastingStream *createWindowStream(const KWayland::Client::PlasmaWindow *window, CursorMode mode);
    ScreencastingStream *createRegionStream(const QRect &geometry, qreal scale, CursorMode mode);
    ScreencastingStream *createVirtualOutputStream(const QString &name, const QString &description, const QSize &size, qreal scale, CursorMode mode);
};
