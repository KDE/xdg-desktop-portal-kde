/*
 * Copyright © 2018-2020 Red Hat, Inc
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

#include <pipewire/pipewire.h>
#include <spa/param/format-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/props.h>

#if PW_CHECK_VERSION(0, 2, 90)
#include <spa/utils/result.h>
#endif

#if !PW_CHECK_VERSION(0, 2, 90)
class PwType {
public:
  spa_type_media_type media_type;
  spa_type_media_subtype media_subtype;
  spa_type_format_video format_video;
  spa_type_video_format video_format;
};
#endif

class ScreenCastStream : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCastStream(const QSize &resolution, QObject *parent = nullptr);
    ~ScreenCastStream();

#if PW_CHECK_VERSION(0, 2, 90)
    static void onCoreError(void *data, uint32_t id, int seq, int res, const char *message);
    static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *format);
#else
    static void onStateChanged(void *data, pw_remote_state old, pw_remote_state state, const char *error);
    static void onStreamFormatChanged(void *data, const struct spa_pod *format);
#endif
    static void onStreamStateChanged(void *data, pw_stream_state old, pw_stream_state state, const char *error_message);
    static void onStreamProcess(void *data);

    // Public
    void init();
    uint framerate();
    uint nodeId();

    // Public because we need access from static functions
    pw_stream *createStream();
    void removeStream();

public Q_SLOTS:
    bool recordFrame(uint8_t *screenData);

Q_SIGNALS:
    void streamReady(uint nodeId);
    void startStreaming();
    void stopStreaming();

#if !PW_CHECK_VERSION(0, 2, 90)
private:
    void initializePwTypes();
#endif

public:
#if PW_CHECK_VERSION(0, 2, 90)
    struct pw_context *pwContext = nullptr;
    struct pw_core *pwCore = nullptr;
    struct pw_stream *pwStream = nullptr;
    struct pw_thread_loop *pwMainLoop = nullptr;

    spa_hook coreListener;
    spa_hook streamListener;

    // event handlers
    pw_core_events pwCoreEvents = {};
    pw_stream_events pwStreamEvents = {};

    uint32_t pwNodeId = 0;
#else
    pw_core *pwCore = nullptr;
    pw_loop *pwLoop = nullptr;
    pw_thread_loop *pwMainLoop = nullptr;
    pw_stream *pwStream = nullptr;
    pw_remote *pwRemote = nullptr;
    pw_type *pwCoreType = nullptr;
    PwType *pwType = nullptr;

    spa_hook remoteListener;
    spa_hook streamListener;

    // event handlers
    pw_remote_events pwRemoteEvents = {};
    pw_stream_events pwStreamEvents = {};
#endif

    QSize resolution;

    spa_video_info_raw videoFormat;

};

#endif // SCREEN_CAST_STREAM_H
