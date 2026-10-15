/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "screencasting.h"
#include <functional>
#include <libei.h>
#include <memory>
#include <vector>

#include <QObject>
#include <QSocketNotifier>

class QEvent;
class QSize;
class QString;
struct ei;
struct ei_touch;

class EiDevice;
class EiRegionDevice;
class ScreencastingStream;

#if defined __cpp_lib_move_only_function && __cpp_lib_move_only_function >= 202110L
template<typename... T>
using function = std::move_only_function<T...>;
#else
template<typename... T>
using function = std::function<T...>;
#endif

class EiConnection : public QObject
{
    Q_OBJECT

public:
    static std::unique_ptr<EiConnection> connect(int fd, const QString &name);
    ~EiConnection() override;

    void requestPointerButtonPress(quint32 linuxButton, bool press);
    void requestPointerButtonRelease(quint32 linuxButton);
    void requestPointerMotion(double x, double y);
    void requestPointerMotionAbsolute(const QString &mappingId, double x, double y);
    void requestPointerAxis(qreal x, qreal y);
    void requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta);

    void requestKeyboardKeycode(int keycode, bool state);
    void requestKeyboardKeysym(int keysym, bool state);

    void requestTouchDown(quint32 touchPoint, const QString &mappingId, double x, double y);
    void requestTouchMotion(quint32 touchPoint, double x, double y);
    void requestTouchUp(quint32 touchPoint);

Q_SIGNALS:
    void error();

private:
    explicit EiConnection(ei *ei);
    void processEisEvents();

    bool setupDone = false;
    std::vector<std::function<void()>> m_pendingRequests;

    QSocketNotifier m_eisNotifier;
    struct ei *m_ei = nullptr;
    std::vector<std::unique_ptr<EiRegionDevice>> m_absolutePointerDevices;
    std::vector<std::unique_ptr<EiRegionDevice>> m_touchDevices;
    std::unique_ptr<EiDevice> m_keyboardDevice;
    std::unique_ptr<EiDevice> m_textDevice;
    std::unique_ptr<EiDevice> m_buttonDevice;
    std::unique_ptr<EiDevice> m_scrollDevice;
    std::unique_ptr<EiDevice> m_relativePointerDevice;
    QHash<int, ei_touch *> touches;
};
