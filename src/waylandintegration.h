/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
#define XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H

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

namespace WaylandIntegration
{
class WaylandOutput
{
public:
    enum OutputType {
        Laptop,
        Monitor,
        Television,
    };
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

class WaylandIntegration : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void newBuffer(uint8_t *screenData);
    void plasmaWindowManagementInitialized();
};

void authenticate();

bool isStreamingEnabled();
bool isStreamingAvailable();

void startStreamingInput();
bool startStreamingOutput(quint32 outputName, Screencasting::CursorMode mode);
bool startStreamingWindow(const QByteArray &winid);
void stopAllStreaming();

void requestPointerButtonPress(quint32 linuxButton);
void requestPointerButtonRelease(quint32 linuxButton);
void requestPointerMotion(const QSizeF &delta);
void requestPointerMotionAbsolute(const QPointF &pos);
void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);

void requestKeyboardKeycode(int keycode, bool state);

QMap<quint32, WaylandOutput> screens();
QVariant streams();

void init();

KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

WaylandIntegration *waylandIntegration();
}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
