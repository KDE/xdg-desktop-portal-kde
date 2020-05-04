/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QVector>
#include <QObject>
#include <QSharedPointer>
#include <optional>

#include "registry.h"
#include <KWayland/Client/kwaylandclient_export.h>

struct zkde_screencast_unstable_v1;

namespace KWayland
{
namespace Client
{
class ScreencastingPrivate;
class ScreencastingSourcePrivate;
class ScreencastingStreamPrivate;
class ScreencastingStream : public QObject
{
    Q_OBJECT
public:
    ScreencastingStream(QObject* parent);
    ~ScreencastingStream() override;

    void close();

Q_SIGNALS:
    void created(quint32 nodeid, const QSize &resolution);
    void failed(const QString &error);
    void closed();

private:
    friend class Screencasting;
    QScopedPointer<ScreencastingStreamPrivate> d;
};

class Screencasting : public QObject
{
    Q_OBJECT
public:
    explicit Screencasting(QObject *parent = nullptr);
    explicit Screencasting(Registry *registry, int id, int version, QObject *parent = nullptr);
    ~Screencasting() override;

    ScreencastingStream* createOutputStream(KWayland::Client::Output* output);
    ScreencastingStream* createWindowStream(quint32 window);

    void setup(zkde_screencast_unstable_v1* screencasting);
    void destroy();

Q_SIGNALS:
    void initialized();
    void removed();
    void sourcesChanged();

private:
    QScopedPointer<ScreencastingPrivate> d;
};

}
}
