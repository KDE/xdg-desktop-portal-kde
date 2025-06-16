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
#include <QFuture>
#include <QObject>
#include <QPoint>
#include <QScreen>
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

class WaylandIntegration : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void newBuffer(uint8_t *screenData);
    void plasmaWindowManagementInitialized();
};

bool isStreamingAvailable();

void acquireStreamingInput(bool acquire);

QFuture<std::unique_ptr<ScreencastingStream>> startStreamingOutput(QScreen *screen, Screencasting::CursorMode mode);
QFuture<std::unique_ptr<ScreencastingStream>> startStreamingWorkspace(Screencasting::CursorMode mode);
QFuture<std::unique_ptr<ScreencastingStream>> startStreamingVirtual(const QString &name, const QString &description, const QSize &size, Screencasting::CursorMode mode);
QFuture<std::unique_ptr<ScreencastingStream>> startStreamingWindow(KWayland::Client::PlasmaWindow *window, Screencasting::CursorMode mode);
QFuture<std::unique_ptr<ScreencastingStream>> startStreamingRegion(const QRect &region, Screencasting::CursorMode mode);

void requestPointerButtonPress(quint32 linuxButton);
void requestPointerButtonRelease(quint32 linuxButton);
void requestPointerMotion(const QSizeF &delta);
void requestPointerMotionAbsolute(ScreencastingStream *const stream, const QPointF &pos);
void requestPointerAxis(qreal x, qreal y);
void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);

void requestKeyboardKeycode(int keycode, bool state);
void requestKeyboardKeysym(int keysym, bool state);

void requestTouchDown(quint32 touchPoint, const QPointF &pos);
void requestTouchMotion(quint32 touchPoint, const QPointF &pos);
void requestTouchUp(quint32 touchPoint);

void init();

KWayland::Client::PlasmaWindowManagement *plasmaWindowManagement();

WaylandIntegration *waylandIntegration();
}

#endif // XDG_DESKTOP_PORTAL_KDE_WAYLAND_INTEGRATION_H
