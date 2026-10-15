/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "eiconnection.h"

#include <QMouseEvent>
#include <QScopeGuard>
#include <QSocketNotifier>
#include <QWheelEvent>

#include <libei.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <utility>

#include "remotedesktop_debug.h"

using namespace Qt::StringLiterals;

class EiDevice
{
public:
    explicit EiDevice(struct ei_device *device)
        : m_device(ei_device_ref(device))
    {
    }

    virtual ~EiDevice()
    {
        ei_device_unref(m_device);
    }

    EiDevice(const EiDevice &) = delete;
    EiDevice &operator=(const EiDevice &) = delete;

    [[nodiscard]] struct ei_device *device() const
    {
        return m_device;
    }

private:
    struct ei_device *m_device = nullptr;
};

class EiRegionDevice : public EiDevice
{
public:
    struct Region {
        QRectF rect;
        qreal scale = 1.0;
    };

    explicit EiRegionDevice(struct ei_device *device)
        : EiDevice(device)
    {
        for (size_t i = 0; auto eiRegion = ei_device_get_region(device, i); ++i) {
            const Region region{
                .rect = QRectF{static_cast<qreal>(ei_region_get_x(eiRegion)),
                               static_cast<qreal>(ei_region_get_y(eiRegion)),
                               static_cast<qreal>(ei_region_get_width(eiRegion)),
                               static_cast<qreal>(ei_region_get_height(eiRegion))},
                .scale = static_cast<qreal>(ei_region_get_physical_scale(eiRegion)),
            };
            const auto mappingId = ei_region_get_mapping_id(eiRegion);
            if (mappingId) {
                m_regions.insert(QString::fromUtf8(mappingId), region);
            }
        }
    }

    EiRegionDevice(const EiRegionDevice &) = delete;
    EiRegionDevice &operator=(const EiRegionDevice &) = delete;

    [[nodiscard]] std::optional<Region> regionForMapping(const QString &mappingId) const
    {
        const auto it = m_regions.constFind(mappingId);
        if (it != m_regions.cend()) {
            return *it;
        }

        return std::nullopt;
    }

private:
    QHash<QString, Region> m_regions;
};

std::unique_ptr<EiConnection> EiConnection::connect(int fd, const QString &name)
{
    ei *ei = ei_new_sender(nullptr);
    ei_configure_name(ei, name.toUtf8().constData());
    if (const auto rc = ei_setup_backend_fd(ei, fd); rc != 0) {
        qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Could not set up libei backend:" << rc;
        ei = ei_unref(ei);
        return nullptr;
    }
    return std::unique_ptr<EiConnection>(new EiConnection(ei));
}

EiConnection::EiConnection(ei *ei)
    : m_eisNotifier(QSocketNotifier(ei_get_fd(ei), QSocketNotifier::Read))
    , m_ei(ei)
{
    QObject::connect(&m_eisNotifier, &QSocketNotifier::activated, this, &EiConnection::processEisEvents);
}

EiConnection::~EiConnection()
{
    if (!m_ei) {
        return;
    }

    ei_disconnect(m_ei);
    while (auto event = ei_get_event(m_ei)) {
        ei_event_unref(event);
    }
    m_ei = ei_unref(m_ei);
}

static std::optional<std::pair<EiRegionDevice *, EiRegionDevice::Region>> findEiDeviceForRegion(const std::vector<std::unique_ptr<EiRegionDevice>> &devices,
                                                                                                const QString &mappingId)
{
    for (const auto &device : devices) {
        const auto region = device->regionForMapping(mappingId);
        if (region.has_value()) {
            return std::pair{device.get(), *region};
        }
    }
    return std::nullopt;
};

void EiConnection::requestPointerButtonPress(quint32 linuxButton, bool press)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, linuxButton, press] {
            requestPointerButtonPress(linuxButton, press);
        });
        return;
    }

    if (m_buttonDevice) {
        ei_device_button_button(m_buttonDevice->device(), linuxButton, press);
        ei_device_frame(m_buttonDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestPointerMotion(double x, double y)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, x, y] {
            requestPointerMotion(x, y);
        });
        return;
    }

    if (m_relativePointerDevice) {
        ei_device_pointer_motion(m_relativePointerDevice->device(), x, y);
        ei_device_frame(m_relativePointerDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestPointerMotionAbsolute(const QString &mappingId, double x, double y)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, mappingId, x, y] {
            requestPointerMotionAbsolute(mappingId, x, y);
        });
        return;
    }

    if (auto device_region = findEiDeviceForRegion(m_absolutePointerDevices, mappingId)) {
        const auto [device, region] = *device_region;
        ei_device_pointer_motion_absolute(device->device(), region.rect.left() + x, region.rect.y() + y);
        ei_device_frame(device->device(), ei_now(m_ei));
    }
}

void EiConnection::requestPointerAxis(qreal x, qreal y)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, x, y] {
            requestPointerAxis(x, y);
        });
        return;
    }

    if (m_scrollDevice) {
        ei_device_scroll_delta(m_scrollDevice->device(), x, y);
        ei_device_frame(m_buttonDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestPointerAxisDiscrete(Qt::Orientation axis, qreal delta)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, axis, delta] {
            requestPointerAxisDiscrete(axis, delta);
        });
        return;
    }

    if (m_scrollDevice) {
        ei_device_scroll_discrete(m_scrollDevice->device(), axis == Qt::Horizontal ? delta * 120 : 0, axis == Qt::Vertical ? delta * 120 : 0);
        ei_device_frame(m_scrollDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestKeyboardKeycode(int keycode, bool state)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, keycode, state] {
            requestKeyboardKeycode(keycode, state);
        });
        return;
    }

    if (m_keyboardDevice) {
        ei_device_keyboard_key(m_keyboardDevice->device(), keycode, state);
        ei_device_frame(m_keyboardDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestKeyboardKeysym(int keysym, bool state)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, keysym, state] {
            requestKeyboardKeysym(keysym, state);
        });
        return;
    }

    if (m_textDevice) {
        ei_device_text_keysym(m_textDevice->device(), keysym, state);
        ei_device_frame(m_textDevice->device(), ei_now(m_ei));
    }
}

void EiConnection::requestTouchDown(quint32 touchPoint, const QString &mappingId, double x, double y)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, touchPoint, mappingId, x, y] {
            requestTouchDown(touchPoint, mappingId, x, y);
        });
        return;
    }

    if (auto device_region = findEiDeviceForRegion(m_touchDevices, mappingId)) {
        const auto [device, region] = *device_region;
        auto touch = *touches.insert(touchPoint, ei_device_touch_new(device->device()));
        ei_touch_down(touch, x, y);
        ei_device_frame(device->device(), ei_now(m_ei));
    }
}
void EiConnection::requestTouchMotion(quint32 touchPoint, double x, double y)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, touchPoint, x, y] {
            requestTouchMotion(touchPoint, x, y);
        });
        return;
    }

    if (auto touch = touches.value(touchPoint, nullptr)) {
        ei_touch_motion(touch, x, y);
        ei_device_frame(ei_touch_get_device(touch), ei_now(m_ei));
    }
}

void EiConnection::requestTouchUp(quint32 touchPoint)
{
    if (!setupDone) {
        m_pendingRequests.emplace_back([this, touchPoint] {
            requestTouchUp(touchPoint);
        });
        return;
    }

    if (auto touch = touches.take(touchPoint)) {
        ei_touch_up(touch);
        ei_device_frame(ei_touch_get_device(touch), ei_now(m_ei));
        ei_touch_unref(touch);
    }
}

void EiConnection::processEisEvents()
{
    ei_dispatch(m_ei);
    while (auto event = ei_get_event(m_ei)) {
        auto cleanup = qScopeGuard([event] {
            ei_event_unref(event);
        });

        const auto eventType = ei_event_get_type(event);
        auto device = ei_event_get_device(event);

        switch (eventType) {
        case EI_EVENT_CONNECT:
            break;
        case EI_EVENT_DISCONNECT:
            qCWarning(XdgDesktopPortalKdeRemoteDesktop) << "Portal EIS connection disconnected";
            m_pendingRequests.clear();
            Q_EMIT error();
            break;
        case EI_EVENT_SEAT_ADDED: {
            auto seat = ei_event_get_seat(event);
            ei_seat_bind_capabilities(seat,
                                      EI_DEVICE_CAP_POINTER,
                                      EI_DEVICE_CAP_POINTER_ABSOLUTE,
                                      EI_DEVICE_CAP_BUTTON,
                                      EI_DEVICE_CAP_SCROLL,
                                      EI_DEVICE_CAP_KEYBOARD,
                                      EI_DEVICE_CAP_TEXT,
                                      nullptr);
            auto ping = ei_new_ping(m_ei);
            ei_ping(ping);
            break;
        }
        case EI_EVENT_DEVICE_ADDED:
            if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE)) {
                m_absolutePointerDevices.push_back(std::make_unique<EiRegionDevice>(device));
            }
            if (ei_device_has_capability(device, EI_DEVICE_CAP_TOUCH)) {
                m_touchDevices.push_back(std::make_unique<EiRegionDevice>(device));
            }
            if (!m_keyboardDevice && ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD)) {
                m_keyboardDevice.reset(new EiDevice(device));
            }
            if (!m_textDevice && ei_device_has_capability(device, EI_DEVICE_CAP_TEXT)) {
                m_textDevice.reset(new EiDevice(device));
            }
            if (!m_relativePointerDevice && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER)) {
                m_relativePointerDevice.reset(new EiDevice(device));
            }
            if (!m_scrollDevice && ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
                m_scrollDevice.reset(new EiDevice(device));
            }
            if (!m_buttonDevice && ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON)) {
                m_buttonDevice.reset(new EiDevice(device));
            }
            break;
        case EI_EVENT_DEVICE_REMOVED:
            std::erase_if(m_absolutePointerDevices, [device](const auto &pointerDevice) {
                return pointerDevice->device() == device;
            });
            std::erase_if(m_touchDevices, [device](const auto &pointerDevice) {
                return pointerDevice->device() == device;
            });
            if (m_keyboardDevice && m_keyboardDevice->device() == device) {
                m_keyboardDevice.reset();
            }
            if (m_textDevice && m_textDevice->device() == device) {
                m_textDevice.reset();
            }
            if (!m_relativePointerDevice && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER)) {
                m_relativePointerDevice.reset();
            }
            if (!m_scrollDevice && ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
                m_scrollDevice.reset();
            }
            if (!m_buttonDevice && ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON)) {
                m_buttonDevice.reset(new EiDevice(device));
            }
            break;
        case EI_EVENT_DEVICE_RESUMED:
            ei_device_start_emulating(device, ei_now(m_ei));
            break;
        case EI_EVENT_PONG:
            ei_ping_unref(ei_event_pong_get_ping(event));
            setupDone = true;
            for (auto &request : m_pendingRequests) {
                request();
            }
            m_pendingRequests.clear();
            break;
        default:
            break;
        }
    }
}

#include "moc_eiconnection.cpp"
