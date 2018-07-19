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

#ifndef SCREEN_CAST_STREAM_H
#define SCREEN_CAST_STREAM_H

#include <QObject>
#include <QSize>

#include <glib-object.h>

#include <spa/support/type-map.h>
#include <spa/param/format-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/raw-utils.h>
#include <spa/param/props.h>

#include <spa/lib/debug.h>

#include <pipewire/factory.h>
#include <pipewire/pipewire.h>
#include <pipewire/remote.h>
#include <pipewire/stream.h>

#ifdef __has_include
#  if __has_include(<pipewire/version.h>)
#    include<pipewire/version.h>
#    define PW_API_PRE_0_2_0 false
#  else
#    define PW_API_PRE_0_2_0 true
#  endif
#else
#define PW_API_PRE_0_2_0 true
#endif

class PwType {
public:
  spa_type_media_type media_type;
  spa_type_media_subtype media_subtype;
  spa_type_format_video format_video;
  spa_type_video_format video_format;
};

class QSocketNotifier;

class ScreenCastStream : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCastStream(const QSize &resolution, QObject *parent = nullptr);
    ~ScreenCastStream();

    // Public
    void init();
    uint nodeId();

    // Public because we need access from static functions
    bool createStream();
    bool recordFrame(uint8_t *screenData);

    void removeStream();
Q_SIGNALS:
    void streamReady(uint nodeId);
    void startStreaming();
    void stopStreaming();

private:
    void initializePwTypes();

private Q_SLOTS:
    void processPipewireEvents();

public:
    pw_core *pwCore = nullptr;
    pw_loop *pwLoop = nullptr;
    pw_node *pwNode = nullptr;
    pw_stream *pwStream = nullptr;
    pw_type *pwCoreType = nullptr;
    pw_remote *pwRemote = nullptr;
    PwType *pwType = nullptr;

    spa_hook remoteListener;
    spa_hook streamListener;

    QSize resolution;
    QScopedPointer<QSocketNotifier> socketNotifier;

    spa_video_info_raw videoFormat;

};

#endif // SCREEN_CAST_STREAM_H


