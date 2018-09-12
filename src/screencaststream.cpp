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

#include "screencaststream.h"
#include "waylandintegration.h"

#include <math.h>
#include <sys/mman.h>
#include <stdio.h>

#include <QLoggingCategory>
#include <QTimer>
#include <QSize>
#include <QSocketNotifier>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeScreenCastStream, "xdp-kde-screencast-stream")

class PwFraction {
public:
    int num;
    int denom;
};

// Stolen from mutter

#define MAX_TERMS       30
#define MIN_DIVISOR     1.0e-10
#define MAX_ERROR       1.0e-20

#define PROP_RANGE(min, max) 2, (min), (max)

#define BITS_PER_PIXEL  4

static int greatestCommonDivisor(int a, int b)
{
    while (b != 0) {
        int temp = a;

        a = b;
        b = temp % b;
    }

    return ABS(a);
}

static PwFraction pipewireFractionFromDouble(double src)
{
    double V, F;                 /* double being converted */
    int N, D;                    /* will contain the result */
    int A;                       /* current term in continued fraction */
    int64_t N1, D1;              /* numerator, denominator of last approx */
    int64_t N2, D2;              /* numerator, denominator of previous approx */
    int i;
    int gcd;
    gboolean negative = FALSE;

    /* initialize fraction being converted */
    F = src;
    if (F < 0.0) {
        F = -F;
        negative = TRUE;
    }

    V = F;
    /* initialize fractions with 1/0, 0/1 */
    N1 = 1;
    D1 = 0;
    N2 = 0;
    D2 = 1;
    N = 1;
    D = 1;

    for (i = 0; i < MAX_TERMS; ++i) {
        /* get next term */
        A = (gint) F;               /* no floor() needed, F is always >= 0 */
        /* get new divisor */
        F = F - A;

        /* calculate new fraction in temp */
        N2 = N1 * A + N2;
        D2 = D1 * A + D2;

        /* guard against overflow */
        if (N2 > G_MAXINT || D2 > G_MAXINT)
            break;

        N = N2;
        D = D2;

        /* save last two fractions */
        N2 = N1;
        D2 = D1;
        N1 = N;
        D1 = D;

        /* quit if dividing by zero or close enough to target */
        if (F < MIN_DIVISOR || fabs (V - ((gdouble) N) / D) < MAX_ERROR)
            break;

        /* Take reciprocal */
        F = 1 / F;
    }

    /* fix for overflow */
    if (D == 0) {
        N = G_MAXINT;
        D = 1;
    }

    /* fix for negative */
    if (negative)
        N = -N;

    /* simplify */
    gcd = greatestCommonDivisor(N, D);
    if (gcd) {
        N /= gcd;
        D /= gcd;
    }

    PwFraction fraction;
    fraction.num = N;
    fraction.denom = D;

    return fraction;
}

static void onStateChanged(void *data, pw_remote_state old, pw_remote_state state, const char *error)
{
    Q_UNUSED(old);

    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

    switch (state) {
    case PW_REMOTE_STATE_ERROR:
        // TODO notify error
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Remote error: " << error;
        break;
    case PW_REMOTE_STATE_CONNECTED:
        // TODO notify error
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Remote state: " << pw_remote_state_as_string(state);
        if (!pw->createStream()) {
            Q_EMIT pw->stoppedStreaming();
        }
        break;
    default:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Remote state: " << pw_remote_state_as_string(state);
        break;
    }
}

static void onStreamStateChanged(void *data, pw_stream_state old, pw_stream_state state, const char *error_message)
{
    Q_UNUSED(old)

    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

    switch (state) {
    case PW_STREAM_STATE_ERROR:
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Stream error: " << error_message;
        break;
    case PW_STREAM_STATE_CONFIGURE:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        Q_EMIT pw->streamReady((uint)pw_stream_get_node_id(pw->pwStream));
        break;
    case PW_STREAM_STATE_UNCONNECTED:
    case PW_STREAM_STATE_CONNECTING:
    case PW_STREAM_STATE_READY:
    case PW_STREAM_STATE_PAUSED:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state) << pw->streaming;
        if (pw->streaming) {
            pw->stopStream();
        }
        break;
    case PW_STREAM_STATE_STREAMING:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        pw->streaming = true;
        WaylandIntegration::startStreaming();
        Q_EMIT pw->startedStreaming();
        break;
    }
}

#if defined(PW_API_PRE_0_2_0)
static void onStreamFormatChanged(void *data, struct spa_pod *format)
#else
static void onStreamFormatChanged(void *data, const struct spa_pod *format)
#endif // defined(PW_API_PRE_0_2_0)
{
    qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream format changed";

    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

    uint8_t paramsBuffer[1024];
    int32_t width, height, stride, size;
    struct spa_pod_builder pod_builder;
#if defined(PW_API_PRE_0_2_0)
    struct spa_pod *params[1];
#else
    const struct spa_pod *params[1];
#endif // defined(PW_API_PRE_0_2_0)
    const int bpp = 4;

    if (!format) {
        pw_stream_finish_format(pw->pwStream, 0, nullptr, 0);
        return;
    }

    spa_format_video_raw_parse (format, &pw->videoFormat, &pw->pwType->format_video);

    width = pw->videoFormat.size.width;
    height =pw->videoFormat.size.height;
    stride = SPA_ROUND_UP_N (width * bpp, 4);
    size = height * stride;

    pod_builder = SPA_POD_BUILDER_INIT (paramsBuffer, sizeof (paramsBuffer));

    params[0] = (spa_pod*) spa_pod_builder_object (&pod_builder,
                                                   pw->pwCoreType->param.idBuffers, pw->pwCoreType->param_buffers.Buffers,
                                                   ":", pw->pwCoreType->param_buffers.size, "i", size,
                                                   ":", pw->pwCoreType->param_buffers.stride, "i", stride,
                                                   ":", pw->pwCoreType->param_buffers.buffers, "iru", 16, PROP_RANGE (2, 16),
                                                   ":", pw->pwCoreType->param_buffers.align, "i", 16);

    pw_stream_finish_format (pw->pwStream, 0,
                             params, G_N_ELEMENTS (params));
}

static const struct pw_remote_events pwRemoteEvents = {
    .version = PW_VERSION_REMOTE_EVENTS,
    .destroy = nullptr,
    .info_changed = nullptr,
    .sync_reply = nullptr,
    .state_changed = onStateChanged,
};

static const struct pw_stream_events pwStreamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = onStreamStateChanged,
    .format_changed = onStreamFormatChanged,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
#if defined(PW_API_PRE_0_2_0)
    .new_buffer = nullptr,
    .need_buffer = nullptr,
#else
    .process = nullptr,
#endif // defined(PW_API_PRE_0_2_0)
};

ScreenCastStream::ScreenCastStream(const QSize &resolution, QObject *parent)
    : QObject(parent)
    , resolution(resolution)
{
}

ScreenCastStream::~ScreenCastStream()
{
    if (pwType) {
        delete pwType;
    }

    if (pwStream) {
        pw_stream_destroy(pwStream);
    }

    if (pwRemote) {
        pw_remote_destroy(pwRemote);
    }

    if (pwCore) {
        pw_core_destroy(pwCore);
    }

    if (pwLoop) {
        pw_loop_leave(pwLoop);
        pw_loop_destroy(pwLoop);
    }
}

void ScreenCastStream::init()
{
    pw_init(nullptr, nullptr);

    pwLoop = pw_loop_new(nullptr);
    socketNotifier.reset(new QSocketNotifier(pw_loop_get_fd(pwLoop), QSocketNotifier::Read));
    connect(socketNotifier.data(), &QSocketNotifier::activated, this, &ScreenCastStream::processPipewireEvents);

    pwCore = pw_core_new(pwLoop, nullptr);
    pwCoreType = pw_core_get_type(pwCore);
    pwRemote = pw_remote_new(pwCore, nullptr, 0);

    initializePwTypes();

    pw_remote_add_listener(pwRemote, &remoteListener, &pwRemoteEvents, this);

    pw_remote_connect(pwRemote);

    connect(WaylandIntegration::waylandIntegration(), &WaylandIntegration::WaylandIntegration::newBuffer, this, &ScreenCastStream::recordFrame);
}

uint ScreenCastStream::nodeId()
{
    if (pwStream) {
        return (uint)pw_stream_get_node_id(pwStream);
    }

    return 0;
}

bool ScreenCastStream::createStream()
{
    if (pw_remote_get_state(pwRemote, nullptr) != PW_REMOTE_STATE_CONNECTED) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Cannot create pipewire stream";
        return false;
    }

    uint8_t buffer[1024];
    spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const float frameRate = 25;

    spa_fraction maxFramerate;
    spa_fraction minFramerate;
    const spa_pod *params[1];

    pwStream = pw_stream_new(pwRemote, "kwin-screen-cast", nullptr);

    PwFraction fraction = pipewireFractionFromDouble(frameRate);

    minFramerate = SPA_FRACTION(1, 1);
    maxFramerate = SPA_FRACTION((uint32_t)fraction.num, (uint32_t)fraction.denom);

    spa_rectangle minResolution = SPA_RECTANGLE(1, 1);
    spa_rectangle maxResolution = SPA_RECTANGLE((uint32_t)resolution.width(), (uint32_t)resolution.height());

    spa_fraction paramFraction = SPA_FRACTION(0, 1);

    params[0] = (spa_pod*)spa_pod_builder_object(&podBuilder,
                                       pwCoreType->param.idEnumFormat, pwCoreType->spa_format,
                                       "I", pwType->media_type.video,
                                       "I", pwType->media_subtype.raw,
                                       ":", pwType->format_video.format, "I", pwType->video_format.xRGB,
                                       ":", pwType->format_video.size, "Rru", &maxResolution, SPA_POD_PROP_MIN_MAX(&minResolution, &maxResolution),
                                       ":", pwType->format_video.framerate, "F", &paramFraction,
                                       ":", pwType->format_video.max_framerate, "Fru", &maxFramerate, PROP_RANGE (&minFramerate, &maxFramerate));

    pw_stream_add_listener(pwStream, &streamListener, &pwStreamEvents, this);

#if defined(PW_API_PRE_0_2_0)
    if (pw_stream_connect(pwStream, PW_DIRECTION_OUTPUT, nullptr, PW_STREAM_FLAG_NONE, params, G_N_ELEMENTS(&params)) != 0) {
#else
    if (pw_stream_connect(pwStream, PW_DIRECTION_OUTPUT, nullptr, static_cast<pw_stream_flags>(PW_STREAM_FLAG_DRIVER | PW_STREAM_FLAG_MAP_BUFFERS), params, G_N_ELEMENTS(&params)) != 0) {
#endif // defined(PW_API_PRE_0_2_0)
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Could not connect to stream";
        return false;
    }

    return true;
}

bool ScreenCastStream::recordFrame(uint8_t *screenData)
{
#if defined(PW_API_PRE_0_2_0)
    uint32_t bufferId;
#else
    struct pw_buffer *buffer;
#endif // defined(PW_API_PRE_0_2_0)
    struct spa_buffer *spa_buffer;
    uint8_t *map = nullptr;
    uint8_t *data = nullptr;

    // TODO check timestamp like mutter does?

    if (!pwStream) {
        return false;
    }

#if defined(PW_API_PRE_0_2_0)
    bufferId = pw_stream_get_empty_buffer(pwStream);

    if (bufferId == SPA_ID_INVALID) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to get empty stream buffer: " << strerror(errno);
        return false;
    }

    spa_buffer = pw_stream_peek_buffer(pwStream, bufferId);
#else
    buffer = pw_stream_dequeue_buffer(pwStream);
#endif // defined(PW_API_PRE_0_2_0)

#if defined(PW_API_PRE_0_2_0)
    if (spa_buffer->datas[0].type == pwCoreType->data.MemFd) {
#else
    spa_buffer = buffer->buffer;

    if (spa_buffer->datas[0].data) {
        data = (uint8_t *) spa_buffer->datas[0].data;
    } else if (spa_buffer->datas[0].type == pwCoreType->data.MemFd) {
#endif // defined(PW_API_PRE_0_2_0)
        map = (uint8_t *)mmap(nullptr, spa_buffer->datas[0].maxsize + spa_buffer->datas[0].mapoffset, PROT_READ | PROT_WRITE, MAP_SHARED, spa_buffer->datas[0].fd, 0);

        if (map == MAP_FAILED) {
            qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to mmap pipewire stream buffer: " << strerror(errno);
            return false;
        }
        data = SPA_MEMBER(map, spa_buffer->datas[0].mapoffset, uint8_t);
#if defined(PW_API_PRE_0_2_0)
    } else if (spa_buffer->datas[0].type == pwCoreType->data.MemPtr) {
        data = (uint8_t *) spa_buffer->datas[0].data;
#endif // defined(PW_API_PRE_0_2_0)
    } else {
        return false;
    }

    memcpy(data, screenData, BITS_PER_PIXEL * videoFormat.size.height * videoFormat.size.width * sizeof(uint8_t));

    if (map) {
        munmap(map, spa_buffer->datas[0].maxsize + spa_buffer->datas[0].mapoffset);
    }

    spa_buffer->datas[0].chunk->size = spa_buffer->datas[0].maxsize;

#if defined(PW_API_PRE_0_2_0)
    pw_stream_send_buffer(pwStream, bufferId);
#else
    pw_stream_queue_buffer(pwStream, buffer);
#endif // defined(PW_API_PRE_0_2_0)
    return true;
}

void ScreenCastStream::removeStream()
{
    // FIXME destroying streams seems to be crashing, Mutter also doesn't remove them, maybe Pipewire does this automatically
    // pw_stream_destroy(pwStream);
    // pwStream = nullptr;
    pw_stream_disconnect(pwStream);
}

void ScreenCastStream::stopStream()
{
    qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stop streaming";
    streaming = false;

    WaylandIntegration::stopStreaming();

    // removeStream();

    Q_EMIT stoppedStreaming();
}

void ScreenCastStream::initializePwTypes()
{
    // raw C-like ScreenCastStream type map
    auto map = pwCoreType->map;

    pwType = new PwType();

    spa_type_media_type_map(map, &pwType->media_type);
    spa_type_media_subtype_map(map, &pwType->media_subtype);
    spa_type_format_video_map (map, &pwType->format_video);
    spa_type_video_format_map (map, &pwType->video_format);
}

void ScreenCastStream::processPipewireEvents()
{
    int result = pw_loop_iterate(pwLoop, 0);
    if (result < 0) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to iterate over pipewire loop: " << spa_strerror(result);
    }
}
