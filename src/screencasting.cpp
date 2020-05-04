/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencasting.h"
#include "qwayland-zkde-screencast-unstable-v1.h"
#include <QRect>
#include <QDebug>
#include <KWayland/Client/output.h>

using namespace KWayland::Client;

class KWayland::Client::ScreencastingStreamPrivate : public QtWayland::zkde_screencast_stream_unstable_v1
{
public:
    ScreencastingStreamPrivate(ScreencastingStream* q) : q(q) {}

    void zkde_screencast_stream_unstable_v1_closed() override {
        Q_EMIT q->closed();
    }

    void zkde_screencast_stream_unstable_v1_created(uint32_t node, quint32 width, quint32 height) override {
        Q_EMIT q->created(node, QSize(width, height));
    }
    void zkde_screencast_stream_unstable_v1_failed(const QString &error) override {
        Q_EMIT q->failed(error);
    }

private:
    ScreencastingStream* const q;
};

ScreencastingStream::ScreencastingStream(QObject* parent)
    : QObject(parent)
    , d(new ScreencastingStreamPrivate(this))
{
}

ScreencastingStream::~ScreencastingStream() = default;

void ScreencastingStream::close()
{
    d->close();
}

class KWayland::Client::ScreencastingPrivate : public QtWayland::zkde_screencast_unstable_v1
{
public:
    ScreencastingPrivate(Registry *registry, int id, int version, Screencasting *q)
        : QtWayland::zkde_screencast_unstable_v1(*registry, id, version)
        , q(q)
    {
    }

    ScreencastingPrivate(::zkde_screencast_unstable_v1* screencasting, Screencasting *q)
        : QtWayland::zkde_screencast_unstable_v1(screencasting)
        , q(q)
    {
    }

    ~ScreencastingPrivate()
    {
        destroy();
    }

    Screencasting *const q;
};

Screencasting::Screencasting(QObject* parent)
    : QObject(parent)
{}

Screencasting::Screencasting(Registry *registry, int id, int version, QObject* parent)
    : QObject(parent)
    , d(new ScreencastingPrivate(registry, id, version, this))
{}

Screencasting::~Screencasting() = default;

ScreencastingStream* Screencasting::createOutputStream(KWayland::Client::Output* output)
{
    auto stream = new ScreencastingStream(this);
    stream->d->init(d->stream_output(*output));
    return stream;
}

ScreencastingStream* Screencasting::createWindowStream(quint32 window)
{
    auto stream = new ScreencastingStream(this);
    stream->d->init(d->stream_window(window));
    return stream;
}

void Screencasting::setup(::zkde_screencast_unstable_v1* screencasting)
{
    d.reset(new ScreencastingPrivate(screencasting, this));
}

void Screencasting::destroy()
{
    d.reset(nullptr);
}
