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

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENCAST_COMMON_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENCAST_COMMON_H

#include <QObject>
#include <QMap>
#include <QSize>

#include "waylandintegration.h"

class ScreenCastStream;

class ScreenCastCommon : public QObject
{
public:
    typedef struct {
        uint nodeId;
        QVariantMap map;
    } Stream;
    typedef QList<Stream> Streams;

    explicit ScreenCastCommon(QObject *parent = nullptr);
    ~ScreenCastCommon();

    QVariant startStreaming(const WaylandIntegration::WaylandOutput &output);

public Q_SLOTS:
    void stopStreaming();

private:
    bool m_streamingEnabled;
    ScreenCastStream *m_stream;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENCAST_COMMON_H


