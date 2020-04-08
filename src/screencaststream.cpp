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

#include "screencaststream.h"

#include <limits.h>
#include <math.h>
#include <sys/mman.h>
#include <stdio.h>

#include <QLoggingCategory>

#include <KNotification>
#include <KLocalizedString>

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

    return qAbs(a);
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
    bool negative = false;

    /* initialize fraction being converted */
    F = src;
    if (F < 0.0) {
        F = -F;
        negative = true;
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
        A = (int) F;               /* no floor() needed, F is always >= 0 */
        /* get new divisor */
        F = F - A;

        /* calculate new fraction in temp */
        N2 = N1 * A + N2;
        D2 = D1 * A + D2;

        /* guard against overflow */
        if (N2 > INT_MAX || D2 > INT_MAX)
            break;

        N = N2;
        D = D2;

        /* save last two fractions */
        N2 = N1;
        D2 = D1;
        N1 = N;
        D1 = D;

        /* quit if dividing by zero or close enough to target */
        if (F < MIN_DIVISOR || fabs (V - ((double) N) / D) < MAX_ERROR)
            break;

        /* Take reciprocal */
        F = 1 / F;
    }

    /* fix for overflow */
    if (D == 0) {
        N = INT_MAX;
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

#if PW_CHECK_VERSION(0, 2, 90)
void ScreenCastStream::onCoreError(void *data, uint32_t id, int seq, int res, const char *message)
{
    Q_UNUSED(seq)
    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

    qCWarning(XdgDesktopPortalKdeScreenCastStream) << "PipeWire remote error: " << message;

    if (id == PW_ID_CORE) {
        if (res == -EPIPE) {
            Q_EMIT pw->stopStreaming();
        }
    }
}
#else
void ScreenCastStream::onStateChanged(void *data, pw_remote_state old, pw_remote_state state, const char *error)
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
        pw->pwStream = pw->createStream();
        if (!pw->pwStream) {
            Q_EMIT pw->stopStreaming();
        }
        break;
    default:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Remote state: " << pw_remote_state_as_string(state);
        break;
    }
}
#endif

void ScreenCastStream::onStreamStateChanged(void *data, pw_stream_state old, pw_stream_state state, const char *error_message)
{
    Q_UNUSED(old)

    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

#if PW_CHECK_VERSION(0, 2, 90)
    switch (state) {
    case PW_STREAM_STATE_ERROR:
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Stream error: " << error_message;
        break;
    case PW_STREAM_STATE_PAUSED:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        if (pw->nodeId() == 0 && pw->pwStream) {
            pw->pwNodeId = pw_stream_get_node_id(pw->pwStream);
            Q_EMIT pw->streamReady(pw->nodeId());
        }
        if (WaylandIntegration::isStreamingEnabled()) {
            Q_EMIT pw->stopStreaming();
        }
        break;
    case PW_STREAM_STATE_STREAMING:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        Q_EMIT pw->startStreaming();
        break;
    case PW_STREAM_STATE_UNCONNECTED:
    case PW_STREAM_STATE_CONNECTING:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        break;
    }
#else
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
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        Q_EMIT pw->stopStreaming();
        break;
    case PW_STREAM_STATE_STREAMING:
        qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream state: " << pw_stream_state_as_string(state);
        Q_EMIT pw->startStreaming();
        break;
    }
#endif
}

#if PW_CHECK_VERSION(0, 2, 90)
void ScreenCastStream::onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *format)
#else
void ScreenCastStream::onStreamFormatChanged(void *data, const struct spa_pod *format)
#endif
{
    qCDebug(XdgDesktopPortalKdeScreenCastStream) << "Stream format changed";

    ScreenCastStream *pw = static_cast<ScreenCastStream*>(data);

    uint8_t paramsBuffer[1024];
    int32_t width, height, stride, size;
    struct spa_pod_builder pod_builder;
    const struct spa_pod *params[1];

#if PW_CHECK_VERSION(0, 2, 90)
    if (!format || id != SPA_PARAM_Format) {
#else
    if (!format) {
        pw_stream_finish_format(pw->pwStream, 0, nullptr, 0);
#endif
        return;
    }

#if PW_CHECK_VERSION(0, 2, 90)
    spa_format_video_raw_parse (format, &pw->videoFormat);
#else
    spa_format_video_raw_parse (format, &pw->videoFormat, &pw->pwType->format_video);
#endif

    width = pw->videoFormat.size.width;
    height =pw->videoFormat.size.height;
    stride = SPA_ROUND_UP_N (width * BITS_PER_PIXEL, 4);
    size = height * stride;

    pod_builder = SPA_POD_BUILDER_INIT (paramsBuffer, sizeof (paramsBuffer));

#if PW_CHECK_VERSION(0, 2, 90)
    params[0] = (spa_pod*) spa_pod_builder_add_object(&pod_builder,
                                                      SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
                                                      SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(16, 2, 16),
                                                      SPA_PARAM_BUFFERS_blocks, SPA_POD_Int (1),
                                                      SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
                                                      SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride),
                                                      SPA_PARAM_BUFFERS_align, SPA_POD_Int(16));
    pw_stream_update_params(pw->pwStream, params, 1);
#else
    params[0] = (spa_pod*) spa_pod_builder_object(&pod_builder,
                                                   pw->pwCoreType->param.idBuffers, pw->pwCoreType->param_buffers.Buffers,
                                                   ":", pw->pwCoreType->param_buffers.size, "i", size,
                                                   ":", pw->pwCoreType->param_buffers.stride, "i", stride,
                                                   ":", pw->pwCoreType->param_buffers.buffers, "iru", 16, PROP_RANGE (2, 16),
                                                   ":", pw->pwCoreType->param_buffers.align, "i", 16);
    pw_stream_finish_format (pw->pwStream, 0, params, 1);
#endif
}

ScreenCastStream::ScreenCastStream(const QSize &resolution, QObject *parent)
    : QObject(parent)
    , resolution(resolution)
{
#if PW_CHECK_VERSION(0, 2, 90)
    pwCoreEvents.version = PW_VERSION_CORE_EVENTS;
    pwCoreEvents.error = &onCoreError;

    pwStreamEvents.version = PW_VERSION_STREAM_EVENTS;
    pwStreamEvents.state_changed = &onStreamStateChanged;
    pwStreamEvents.param_changed = &onStreamParamChanged;
#else
    // initialize event handlers, remote end and stream-related
    pwRemoteEvents.version = PW_VERSION_REMOTE_EVENTS;
    pwRemoteEvents.state_changed = &onStateChanged;

    pwStreamEvents.version = PW_VERSION_STREAM_EVENTS;
    pwStreamEvents.state_changed = &onStreamStateChanged;
    pwStreamEvents.format_changed = &onStreamFormatChanged;
#endif
}

ScreenCastStream::~ScreenCastStream()
{
    if (pwMainLoop) {
        pw_thread_loop_stop(pwMainLoop);
    }

#if !PW_CHECK_VERSION(0, 2, 90)
    if (pwType) {
        delete pwType;
    }
#endif

    if (pwStream) {
        pw_stream_destroy(pwStream);
    }

#if !PW_CHECK_VERSION(0, 2, 90)
    if (pwRemote) {
        pw_remote_destroy(pwRemote);
    }
#endif

#if PW_CHECK_VERSION(0, 2, 90)
    if (pwCore) {
        pw_core_disconnect(pwCore);
    }

    if (pwContext) {
        pw_context_destroy(pwContext);
    }
#else
    if (pwCore) {
        pw_core_destroy(pwCore);
    }
#endif

    if (pwMainLoop) {
        pw_thread_loop_destroy(pwMainLoop);
    }

#if !PW_CHECK_VERSION(0, 2, 90)
    if (pwLoop) {
        pw_loop_leave(pwLoop);
        pw_loop_destroy(pwLoop);
    }
#endif
}

void ScreenCastStream::init()
{
    pw_init(nullptr, nullptr);

    const auto emitFailureNotification = [](const QString &body) {
        KNotification *notification = new KNotification(QStringLiteral("screencastfailure"), KNotification::CloseOnTimeout);
        notification->setTitle(i18n("Failed to start screencasting"));
        notification->setText(body);
        notification->setIconName(QStringLiteral("dialog-error"));
        notification->sendEvent();
    };

#if PW_CHECK_VERSION(0, 2, 90)
    pwMainLoop = pw_thread_loop_new("pipewire-main-loop", nullptr);
    pwContext = pw_context_new(pw_thread_loop_get_loop(pwMainLoop), nullptr, 0);
    if (!pwContext) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to create PipeWire context";
        emitFailureNotification(i18n("Failed to create PipeWire context"));
        return;
    }

    pwCore = pw_context_connect(pwContext, nullptr, 0);
    if (!pwCore) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to connect PipeWire context";
        emitFailureNotification(i18n("Failed to connect PipeWire context"));
        return;
    }

    pw_core_add_listener(pwCore, &coreListener, &pwCoreEvents, this);

    pwStream = createStream();
    if (!pwStream) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to create PipeWire stream";
        emitFailureNotification(i18n("Failed to create PipeWire stream"));
        return;
    }
#else
    pwLoop = pw_loop_new(nullptr);
    pwMainLoop = pw_thread_loop_new(pwLoop, "pipewire-main-loop");
    pwCore = pw_core_new(pwLoop, nullptr);
    pwCoreType = pw_core_get_type(pwCore);
    pwRemote = pw_remote_new(pwCore, nullptr, 0);

    initializePwTypes();

    pw_remote_add_listener(pwRemote, &remoteListener, &pwRemoteEvents, this);
    pw_remote_connect(pwRemote);
#endif


    if (pw_thread_loop_start(pwMainLoop) < 0) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to start main PipeWire loop";
        emitFailureNotification(i18n("Failed to start main PipeWire loop"));
        return;
    }
}

uint ScreenCastStream::framerate()
{
    if (pwStream) {
        return videoFormat.max_framerate.num / videoFormat.max_framerate.denom;
    }

    return 0;
}

uint ScreenCastStream::nodeId()
{
#if PW_CHECK_VERSION(0, 2, 90)
    return pwNodeId;
#else
    if (pwStream) {
        return (uint)pw_stream_get_node_id(pwStream);
    }

    return 0;
#endif
}

pw_stream *ScreenCastStream::createStream()
{
#if !PW_CHECK_VERSION(0, 2, 90)
    if (pw_remote_get_state(pwRemote, nullptr) != PW_REMOTE_STATE_CONNECTED) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Cannot create pipewire stream";
        return nullptr;
    }
#endif

    uint8_t buffer[1024];
    spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const float frameRate = 25;

    spa_fraction maxFramerate;
    spa_fraction minFramerate;
    const spa_pod *params[1];

#if PW_CHECK_VERSION(0, 2, 90)
    auto stream = pw_stream_new(pwCore, "kwin-screen-cast", nullptr);
#else
    auto stream = pw_stream_new(pwRemote, "kwin-screen-cast", nullptr);
#endif

    PwFraction fraction = pipewireFractionFromDouble(frameRate);

    minFramerate = SPA_FRACTION(1, 1);
    maxFramerate = SPA_FRACTION((uint32_t)fraction.num, (uint32_t)fraction.denom);

    spa_rectangle minResolution = SPA_RECTANGLE(1, 1);
    spa_rectangle maxResolution = SPA_RECTANGLE((uint32_t)resolution.width(), (uint32_t)resolution.height());

    spa_fraction paramFraction = SPA_FRACTION(0, 1);

#if PW_CHECK_VERSION(0, 2, 90)
    params[0] = (spa_pod*)spa_pod_builder_add_object(&podBuilder,
                                        SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
                                        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
                                        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                        SPA_FORMAT_VIDEO_format, SPA_POD_Id(SPA_VIDEO_FORMAT_RGBx),
                                        SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&maxResolution, &minResolution, &maxResolution),
                                        SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&paramFraction),
                                        SPA_FORMAT_VIDEO_maxFramerate, SPA_POD_CHOICE_RANGE_Fraction(&maxFramerate, &minFramerate, &maxFramerate));
#else
    params[0] = (spa_pod*)spa_pod_builder_object(&podBuilder,
                                        pwCoreType->param.idEnumFormat, pwCoreType->spa_format,
                                        "I", pwType->media_type.video,
                                        "I", pwType->media_subtype.raw,
                                        ":", pwType->format_video.format, "I", pwType->video_format.RGBx,
                                        ":", pwType->format_video.size, "Rru", &maxResolution, SPA_POD_PROP_MIN_MAX(&minResolution, &maxResolution),
                                        ":", pwType->format_video.framerate, "F", &paramFraction,
                                        ":", pwType->format_video.max_framerate, "Fru", &maxFramerate, PROP_RANGE (&minFramerate, &maxFramerate));
#endif

    pw_stream_add_listener(stream, &streamListener, &pwStreamEvents, this);

    auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_DRIVER | PW_STREAM_FLAG_MAP_BUFFERS);

#if PW_CHECK_VERSION(0, 2, 90)
    if (pw_stream_connect(stream, PW_DIRECTION_OUTPUT, SPA_ID_INVALID, flags, params, 1) != 0) {
#else
    if (pw_stream_connect(stream, PW_DIRECTION_OUTPUT, nullptr, flags, params, 1) != 0) {
#endif
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Could not connect to stream";
        return nullptr;
    }

    return stream;
}

bool ScreenCastStream::recordFrame(gbm_bo *bo, quint32 width, quint32 height, quint32 stride)
{
    struct pw_buffer *buffer;
    struct spa_buffer *spa_buffer;
    uint8_t *data = nullptr;

    if (!(buffer = pw_stream_dequeue_buffer(pwStream))) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to record frame: couldn't obtain PipeWire buffer";
        return false;
    }

    spa_buffer = buffer->buffer;

    if (!(data = (uint8_t *) spa_buffer->datas[0].data)) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to record frame: invalid buffer data";
        return false;
    }

    const quint32 destStride = SPA_ROUND_UP_N(videoFormat.size.width * BITS_PER_PIXEL, 4);
    const quint32 destSize = BITS_PER_PIXEL * width * height * sizeof(uint8_t);
    const quint32 srcSize = spa_buffer->datas[0].maxsize;

    if (destSize != srcSize || stride != destStride) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to record frame: different stride";
        return false;
    }

    // bind context to render thread
    eglMakeCurrent(WaylandIntegration::egl().display, EGL_NO_SURFACE, EGL_NO_SURFACE, WaylandIntegration::egl().context);

    // create EGL image from imported BO
    EGLImageKHR image = eglCreateImageKHR(WaylandIntegration::egl().display, nullptr, EGL_NATIVE_PIXMAP_KHR, bo, nullptr);

    if (image == EGL_NO_IMAGE_KHR) {
        qCWarning(XdgDesktopPortalKdeScreenCastStream) << "Failed to record frame: Error creating EGLImageKHR - " << WaylandIntegration::formatGLError(glGetError());
        return false;
    }

    // create GL 2D texture for framebuffer
    GLuint texture;
    glGenTextures(1, &texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glDeleteTextures(1, &texture);
    eglDestroyImageKHR(WaylandIntegration::egl().display, image);

    spa_buffer->datas[0].chunk->offset = 0;
    spa_buffer->datas[0].chunk->size = spa_buffer->datas[0].maxsize;
    spa_buffer->datas[0].chunk->stride = destStride;

    pw_stream_queue_buffer(pwStream, buffer);
    return true;
}

void ScreenCastStream::removeStream()
{
    // FIXME destroying streams seems to be crashing, Mutter also doesn't remove them, maybe Pipewire does this automatically
    // pw_stream_destroy(pwStream);
    // pwStream = nullptr;
    pw_stream_disconnect(pwStream);
}

#if !PW_CHECK_VERSION(0, 2, 90)
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
#endif
