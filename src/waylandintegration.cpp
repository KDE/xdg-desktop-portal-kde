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

#include "waylandintegration.h"
#include "waylandintegration_p.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>
#include <QTimer>

#include <QImage>

// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/output.h>
#include <KWayland/Client/remote_access.h>

// system
#include <fcntl.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeWaylandIntegration, "xdp-kde-wayland-integration")

Q_GLOBAL_STATIC(WaylandIntegration::WaylandIntegrationPrivate, globalWaylandIntegration)

void WaylandIntegration::init()
{
    globalWaylandIntegration->initDrm();
    globalWaylandIntegration->initEGL();
    globalWaylandIntegration->initWayland();
}

void WaylandIntegration::bindOutput(int outputName, int outputVersion)
{
    globalWaylandIntegration->bindOutput(outputName, outputVersion);
}

void WaylandIntegration::startStreaming()
{
    globalWaylandIntegration->startStreaming();
}

void WaylandIntegration::stopStreaming()
{
    globalWaylandIntegration->stopStreaming();
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::screens()
{
    return globalWaylandIntegration->screens();
}

WaylandIntegration::WaylandIntegration * WaylandIntegration::waylandIntegration()
{
    return globalWaylandIntegration;
}

static const char * formatGLError(GLenum err)
{
    switch(err) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return (QLatin1String("0x") + QString::number(err, 16)).toLocal8Bit().constData();
    }
}

// Thank you kscreen
void WaylandIntegration::WaylandOutput::setOutputType(const QString &type)
{
    const auto embedded = { QLatin1String("LVDS"),
                                   QLatin1String("IDP"),
                                   QLatin1String("EDP"),
                                   QLatin1String("LCD") };

    for (const QLatin1String &pre : embedded) {
        if (type.toUpper().startsWith(pre)) {
            m_outputType = OutputType::Laptop;
            return;
        }
    }

    if (type.contains("VGA") || type.contains("DVI") || type.contains("HDMI") || type.contains("Panel") ||
        type.contains("DisplayPort") || type.startsWith("DP") || type.contains("unknown")) {
        m_outputType = OutputType::Monitor;
    } else if (type.contains("TV")) {
        m_outputType = OutputType::Television;
    } else {
        m_outputType = OutputType::Monitor;
    }
}

WaylandIntegration::WaylandIntegrationPrivate::WaylandIntegrationPrivate()
    : WaylandIntegration()
    , m_registryInitialized(false)
    , m_connection(nullptr)
    , m_queue(nullptr)
    , m_registry(nullptr)
    , m_remoteAccessManager(nullptr)
{
}

WaylandIntegration::WaylandIntegrationPrivate::~WaylandIntegrationPrivate()
{
    if (m_remoteAccessManager) {
        m_remoteAccessManager->destroy();
    }

    if (m_drmFd) {
        gbm_device_destroy(m_gbmDevice);
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::bindOutput(int outputName, int outputVersion)
{
    KWayland::Client::Output *output = new KWayland::Client::Output(this);
    output->setup(m_registry->bindOutput(outputName, outputVersion));
    m_bindOutputs << output;
}

void WaylandIntegration::WaylandIntegrationPrivate::startStreaming()
{
    m_streamingEnabled = true;

    if (!m_registryInitialized) {
        qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "Cannot start stream because registry is not initialized yet";
        return;
    }
    if (m_registry->hasInterface(KWayland::Client::Registry::Interface::RemoteAccessManager)) {
        KWayland::Client::Registry::AnnouncedInterface interface = m_registry->interface(KWayland::Client::Registry::Interface::RemoteAccessManager);
        if (!interface.name && !interface.version) {
            qCWarning(XdgDesktopPortalKdeWaylandIntegration) << "Cannot start stream because remote access interface is not initialized yet";
            return;
        }
        m_remoteAccessManager = m_registry->createRemoteAccessManager(interface.name, interface.version);
        connect(m_remoteAccessManager, &KWayland::Client::RemoteAccessManager::bufferReady, this, [this] (const void *output, const KWayland::Client::RemoteBuffer * rbuf) {
            Q_UNUSED(output);
            connect(rbuf, &KWayland::Client::RemoteBuffer::parametersObtained, this, [this, rbuf] {
                processBuffer(rbuf);
            });
        });
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::stopStreaming()
{
    m_remoteAccessManager->release();
    m_remoteAccessManager->destroy();

    m_streamingEnabled = false;

    qDeleteAll(m_bindOutputs);
    m_bindOutputs.clear();
}

QMap<quint32, WaylandIntegration::WaylandOutput> WaylandIntegration::WaylandIntegrationPrivate::screens()
{
    return m_outputMap;
}

void WaylandIntegration::WaylandIntegrationPrivate::initDrm()
{
    m_drmFd = open("/dev/dri/renderD128", O_RDWR);
    m_gbmDevice = gbm_create_device(m_drmFd);

    if (!m_gbmDevice) {
        qFatal("Cannot create GBM device: %s", strerror(errno));
    }
}

void WaylandIntegration::WaylandIntegrationPrivate::initEGL()
{
    // Get the list of client extensions
    const char* clientExtensionsCString = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    const QByteArray clientExtensionsString = QByteArray::fromRawData(clientExtensionsCString, qstrlen(clientExtensionsCString));
    if (clientExtensionsString.isEmpty()) {
        // If eglQueryString() returned NULL, the implementation doesn't support
        // EGL_EXT_client_extensions. Expect an EGL_BAD_DISPLAY error.
        qFatal("No client extensions defined! %s", formatGLError(eglGetError()));
    }

    m_egl.extensions = clientExtensionsString.split(' ');

    // Use eglGetPlatformDisplayEXT() to get the display pointer
    // if the implementation supports it.
    if (!m_egl.extensions.contains(QByteArrayLiteral("EGL_EXT_platform_base")) ||
            !m_egl.extensions.contains(QByteArrayLiteral("EGL_MESA_platform_gbm"))) {
        qFatal("One of required EGL extensions is missing");
    }

    m_egl.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, m_gbmDevice, nullptr);

    if (m_egl.display == EGL_NO_DISPLAY) {
        qFatal("Error during obtaining EGL display: %s", formatGLError(eglGetError()));
    }

    EGLint major, minor;
    if (eglInitialize(m_egl.display, &major, &minor) == EGL_FALSE) {
        qFatal("Error during eglInitialize: %s", formatGLError(eglGetError()));
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        qFatal("bind OpenGL API failed");
    }

    m_egl.context = eglCreateContext(m_egl.display, nullptr, EGL_NO_CONTEXT, nullptr);

    if (m_egl.context == EGL_NO_CONTEXT) {
        qFatal("Couldn't create EGL context: %s", formatGLError(eglGetError()));
    }

    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Egl initialization succeeded";
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << QString("EGL version: %1.%2").arg(major).arg(minor);
}

void WaylandIntegration::WaylandIntegrationPrivate::initWayland()
{
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "InitWayland()";

    m_thread = new QThread(this);
    m_connection = new KWayland::Client::ConnectionThread;

    connect(m_connection, &KWayland::Client::ConnectionThread::connected, this, &WaylandIntegrationPrivate::setupRegistry, Qt::QueuedConnection);
    connect(m_connection, &KWayland::Client::ConnectionThread::connectionDied, this, [this] {
        if (m_queue) {
            delete m_queue;
            m_queue = nullptr;
        }

        m_connection->deleteLater();
        m_connection = nullptr;

        if (m_thread) {
            m_thread->quit();
            if (!m_thread->wait(3000)) {
                m_thread->terminate();
                m_thread->wait();
            }
            delete m_thread;
            m_thread = nullptr;
        }
    });
    connect(m_connection, &KWayland::Client::ConnectionThread::failed, this, [this] {
        m_thread->quit();
        m_thread->wait();
    });

    m_thread->start();
    m_connection->moveToThread(m_thread);
    m_connection->initConnection();
}

void WaylandIntegration::WaylandIntegrationPrivate::addOutput(quint32 name, quint32 version)
{
    KWayland::Client::Output *output = new KWayland::Client::Output(this);
    output->setup(m_registry->bindOutput(name, version));

    connect(output, &KWayland::Client::Output::changed, this, [this, name, version, output] () {
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Adding output:";
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    manufacturer: " << output->manufacturer();
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    model: " << output->model();
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    resolution: " << output->pixelSize();

        WaylandOutput portalOutput;
        portalOutput.setManufacturer(output->manufacturer());
        portalOutput.setModel(output->model());
        portalOutput.setOutputType(output->model());
        portalOutput.setResolution(output->pixelSize());
        portalOutput.setWaylandOutputName(name);
        portalOutput.setWaylandOutputVersion(version);

        m_outputMap.insert(name, portalOutput);

        delete output;
    });
}

void WaylandIntegration::WaylandIntegrationPrivate::removeOutput(quint32 name)
{
    WaylandOutput output = m_outputMap.take(name);
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Removing output:";
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    manufacturer: " << output.manufacturer();
    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "    model: " << output.model();
}

void WaylandIntegration::WaylandIntegrationPrivate::processBuffer(const KWayland::Client::RemoteBuffer* rbuf)
{
    QScopedPointer<const KWayland::Client::RemoteBuffer> guard(rbuf);

    auto gbmHandle = rbuf->fd();
    auto width = rbuf->width();
    auto height = rbuf->height();
    auto stride = rbuf->stride();
    auto format = rbuf->format();

    qCDebug(XdgDesktopPortalKdeWaylandIntegration) << QString("Incoming GBM fd %1, %2x%3, stride %4, fourcc 0x%5").arg(gbmHandle).arg(width).arg(height).arg(stride).arg(QString::number(format, 16));

    if (!m_streamingEnabled) {
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Streaming is disabled";
        close(gbmHandle);
        return;
    }

    if (!gbm_device_is_format_supported(m_gbmDevice, format, GBM_BO_USE_SCANOUT)) {
        qCritical() << "GBM format is not supported by device!";
    }

    // import GBM buffer that was passed from KWin
    gbm_import_fd_data importInfo = {gbmHandle, width, height, stride, format};
    gbm_bo *imported = gbm_bo_import(m_gbmDevice, GBM_BO_IMPORT_FD, &importInfo, GBM_BO_USE_SCANOUT);
    if (!imported) {
        qCritical() << "Cannot import passed GBM fd:" << strerror(errno);
    }

    // bind context to render thread
    eglMakeCurrent(m_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_egl.context);

    // create EGL image from imported BO
    EGLImageKHR image = eglCreateImageKHR(m_egl.display, nullptr, EGL_NATIVE_PIXMAP_KHR, imported, nullptr);
    if (image == EGL_NO_IMAGE_KHR) {
        qCritical() << "Error creating EGLImageKHR" << formatGLError(glGetError());
        return;
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

    // bind framebuffer to copy pixels from
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qCritical() << "glCheckFramebufferStatus failed:" << formatGLError(glGetError());
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &framebuffer);
        eglDestroyImageKHR(m_egl.display, image);
        return;
    }

    auto capture = new QImage(QSize(width, height), QImage::Format_RGBA8888);
    glViewport(0, 0, width, height);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, capture->bits());

    Q_EMIT newBuffer(capture->bits());

    gbm_bo_destroy(imported);
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &framebuffer);
    eglDestroyImageKHR(m_egl.display, image);

    delete capture;

    close(gbmHandle);
}

void WaylandIntegration::WaylandIntegrationPrivate::setupRegistry()
{
    m_queue = new KWayland::Client::EventQueue(this);
    m_queue->setup(m_connection);

    m_registry = new KWayland::Client::Registry(this);

    connect(m_registry, &KWayland::Client::Registry::outputAnnounced, this, &WaylandIntegrationPrivate::addOutput);
    connect(m_registry, &KWayland::Client::Registry::outputRemoved, this, &WaylandIntegrationPrivate::removeOutput);

    connect(m_registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this] {
        m_registryInitialized = true;
        qCDebug(XdgDesktopPortalKdeWaylandIntegration) << "Registry initialized";
    });

    m_registry->create(m_connection);
    m_registry->setEventQueue(m_queue);
    m_registry->setup();
}
