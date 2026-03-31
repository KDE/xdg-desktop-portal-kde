/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencasting.h"
#include "screencast.h"
#include "screencast_debug.h"

#include <KWayland/Client/plasmawindowmanagement.h>

#include <QRect>

#include <QGuiApplication>

constexpr unsigned int version = 1;

ScreencastingStream::ScreencastingStream(::kde_screencast_stream_v2 *object)
    : kde_screencast_stream_v2(object)
{
}

ScreencastingStream::~ScreencastingStream()
{
    destroy();
}

void ScreencastingStream::kde_screencast_stream_v2_created(uint32_t node, uint32_t object_serial_hi, uint32_t object_serial_low)
{
    nodeId = node;
    objectSerial = quint64(object_serial_hi) << 32 | object_serial_low;
    Q_EMIT created();
}

void ScreencastingStream::kde_screencast_stream_v2_failed(const QString &error)
{
    Q_EMIT failed(error);
}

void ScreencastingStream::kde_screencast_stream_v2_closed()
{
    Q_EMIT closed();
}

Screencasting::Screencasting()
    : QWaylandClientExtensionTemplate(::version)
{
    initialize();
}

Screencasting::~Screencasting() = default;

ScreencastingStream *Screencasting::createOutputStream(QScreen *screen, CursorMode mode)
{
    QtWayland::kde_screencast_output_params_v2 params(stream_output(screen->name()));
    params.set_pointer_mode(mode);
    auto stream = new ScreencastingStream(params.create_stream());

    stream->geometry = screen->geometry();
    connect(screen, &QScreen::geometryChanged, stream, [stream](const QRect &geometry) {
        stream->geometry = geometry;
    });
    stream->metaData = {
        {QLatin1String("position"), screen->geometry().topLeft()},
        {QLatin1String("size"), screen->size()},
        {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Monitor)},
    };
    return stream;
}

ScreencastingStream *Screencasting::createWindowStream(const KWayland::Client::PlasmaWindow *window, CursorMode mode)
{
    QtWayland::kde_screencast_window_params_v2 params(stream_window(QString::fromUtf8(window->uuid())));
    params.set_pointer_mode(mode);
    auto stream = new ScreencastingStream(params.create_stream());

    stream->geometry = window->geometry();
    connect(window, &KWayland::Client::PlasmaWindow::geometryChanged, stream, [window, stream] {
        stream->geometry = window->geometry();
    });

    stream->metaData = {{QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Window)}};
    return stream;
}

ScreencastingStream *Screencasting::createRegionStream(const QRect &g, qreal scale, CursorMode mode)
{
    QtWayland::kde_screencast_region_params_v2 params(stream_region(g.x(), g.y(), g.width(), g.height()));
    params.set_scale(wl_fixed_from_double(scale));
    params.set_pointer_mode(mode);
    auto stream = new ScreencastingStream(params.create_stream());

    stream->geometry = g;
    stream->metaData = {
        {QLatin1String("size"), g.size()},
        {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Monitor)},
    };
    return stream;
}

ScreencastingStream *
Screencasting::createVirtualOutputStream(const QString &name, const QString &description, const QSize &s, qreal scale, Screencasting::CursorMode mode)
{
    QtWayland::kde_screencast_virtual_output_params_v2 params(stream_virtual_output(s.width(), s.height(), wl_fixed_from_double(scale)));
    params.set_name(name);
    params.set_description(description);
    auto stream = new ScreencastingStream(params.create_stream());

    connect(qGuiApp, &QGuiApplication::screenAdded, stream, [stream, name](const QScreen *screen) {
        // KWin adds "Virtual-" to virtual screen naems
        if (screen->name() == QLatin1StringView("Virtual-") + name) {
            stream->geometry = screen->geometry();
            connect(screen, &QScreen::geometryChanged, stream, [stream](const QRect &geometry) {
                stream->geometry = geometry;
            });
        }
    });
    stream->metaData = {
        {QLatin1String("size"), s},
        {QLatin1String("source_type"), static_cast<uint>(ScreenCastPortal::Virtual)},
    };
    return stream;
}

#include "moc_screencasting.cpp"
