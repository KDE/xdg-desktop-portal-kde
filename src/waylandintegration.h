/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
#define XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H

#include <QDBusArgument>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QVariant>

#include <KWayland/Client/output.h>
#include <screencasting.h>

namespace KWayland
{
namespace Client
{
class PlasmaWindowManagement;
class ScreencastingSource;
}
}

class QWindow;

namespace WaylandIntegration
{
class WaylandOutput
{
    Q_GADGET
public:
    enum OutputType {
        Laptop,
        Monitor,
        Television,
        Workspace,
        Virtual,
    };
    Q_ENUM(OutputType)
    QString manufacturer() const
    {
        return m_output->manufacturer();
    }
    QString model() const
    {
        return m_output->model();
    }
    QPoint globalPosition() const
    {
        return m_output->globalPosition();
    }
    QSize resolution() const
    {
        return m_output->pixelSize();
    }
    OutputType outputType() const
    {
        return m_outputType;
    }

    QSharedPointer<KWayland::Client::Output> output() const
    {
        return m_output;
    }
    void setOutput(const QSharedPointer<KWayland::Client::Output> &output)
    {
        m_output = output;
        setOutputType(output->model());
    }

    void setWaylandOutputName(int outputName)
    {
        m_waylandOutputName = outputName;
    }
    int waylandOutputName() const
    {
        return m_waylandOutputName;
    }

    void setWaylandOutputVersion(int outputVersion)
    {
        m_waylandOutputVersion = outputVersion;
    }
    int waylandOutputVersion() const
    {
        return m_waylandOutputVersion;
    }

private:
    void setOutputType(const QString &model);
    OutputType m_outputType = Monitor;
    QSharedPointer<KWayland::Client::Output> m_output;

    // Needed for later output binding
    int m_waylandOutputName;
    int m_waylandOutputVersion;
};

struct Stream {
    ScreencastingStream *stream = nullptr;
    uint nodeId;
    QVariantMap map;
    bool isValid() const
    {
        return stream != nullptr;
    }

    void close();
};
typedef QVector<Stream> Streams;

class WaylandIntegration : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void newBuffer(uint8_t *screenData);
    void plasmaWindowManagementInitialized();
    void streamingStopped();
};

void authenticate();

bool isStreamingEnabled();
bool isStreamingAvailable();

void startStreamingInput();
Stream startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode);
Stream startStreamingWorkspace(Screencasting::CursorMode mode);
Stream startStreamingVirtual(const QString &name, const QSize &size, Screencasting::CursorMode mode);
Stream startStreamingWindow(const QMap<int, QVariant> &win);
void stopAllStreaming();
void stopStreaming(uint node);

void requestPointerButtonPress(quint32 linuxButton);
void requestPointerButtonRelease(quint32 linuxButton);
void requestPointerMotion(const QSizeF &delta);
void requestPointerMotionAbsolute(const QPointF &pos);
void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);

void requestKeyboardKeycode(int keycode, bool state);

void requestTouchDown(quint32 touchPoint, const QPointF &pos);
void requestTouchMotion(quint32 touchPoint, const QPointF &pos);
void requestTouchUp(quint32 touchPoint);

QMap<quint32, WaylandOutput> screens();

void setParentWindow(QWindow *window, const QString &parentWindow);

void init();

KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

WaylandIntegration *waylandIntegration();

QDebug operator<<(QDebug dbg, const Stream &c);

const QDBusArgument &operator<<(QDBusArgument &arg, const Stream &stream);
const QDBusArgument &operator>>(const QDBusArgument &arg, Stream &stream);
}

Q_DECLARE_METATYPE(WaylandIntegration::Stream)
Q_DECLARE_METATYPE(WaylandIntegration::Streams)

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
