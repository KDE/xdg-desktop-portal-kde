/*
    SPDX-FileCopyrightText: 2018-2020 Red Hat Inc
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
    SPDX-FileContributor: Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pipewirecore.h"
#include <cerrno>
#include <pipewire/node.h>

#include <KLocalizedString>

#include <QSocketNotifier>

PipeWireCore::PipeWireCore()
{
    pw_init(nullptr, nullptr);

    pwMainLoop.reset(pw_loop_new(nullptr));
    if (!pwMainLoop) {
        qWarning("Failed to create PipeWire loop: %s", strerror(errno));
        return;
    }
    pw_loop_enter(pwMainLoop.get());

    QSocketNotifier *notifier = new QSocketNotifier(pw_loop_get_fd(pwMainLoop.get()), QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this] {
        int result = pw_loop_iterate(pwMainLoop, 0);
        if (result < 0) {
            qWarning() << "pipewire_loop_iterate failed: " << result;
        }
    });

    pwContext.reset(pw_context_new(pwMainLoop.get(), nullptr, 0));
    if (!pwContext) {
        qWarning() << "Failed to create PipeWire context";
        return;
    }

    pwCore.reset(pw_context_connect(pwContext.get(), nullptr, 0));
    if (!pwCore) {
        qWarning() << "Failed to connect PipeWire context";
        return;
    }

    if (pw_loop_iterate(pwMainLoop, 0) < 0) {
        qWarning() << "Failed to start main PipeWire loop";
        return;
    }

    static constexpr auto pwCoreEvents = [] {
        pw_core_events events = {};
        events.version = PW_VERSION_CLIENT;
        events.error = &PipeWireCore::core_error;
        return events;
    }();

    pw_core_add_listener(pwCore.get(), &coreListener, &pwCoreEvents, this);

    pwRegistry.reset(pw_core_get_registry(pwCore.get(), PW_VERSION_REGISTRY, 0));
    if (!pwRegistry) {
        qWarning() << "Failed to create pipewire registry";
    }

    static constexpr pw_registry_events pwRegistryEvents = {.version = PW_VERSION_REGISTRY,
                                                            .global = &PipeWireCore::registry_global,
                                                            .global_remove = &PipeWireCore::registry_global_remove};

    pw_registry_add_listener(pwRegistry.get(), &registryListener, &pwRegistryEvents, this);

    m_valid = true;
}

PipeWireCore::~PipeWireCore()
{
    if (pwMainLoop) {
        pw_loop_leave(pwMainLoop);
    }

    pw_deinit();
}

void PipeWireCore::core_error(void *data, uint32_t id, [[maybe_unused]] int seq, int res, const char *message)
{
    qWarning() << "PipeWire remote error: " << message;
    if (id == PW_ID_CORE && res == -EPIPE) {
        PipeWireCore *pw = static_cast<PipeWireCore *>(data);
        pw->m_valid = false;
    }
}

void PipeWireCore::registry_global(void *data,
                                   uint32_t id,
                                   [[maybe_unused]] uint32_t permissions,
                                   const char *type,
                                   [[maybe_unused]] uint32_t version,
                                   [[maybe_unused]] const struct spa_dict *props)
{
    static constexpr pw_node_events pwNodeEvents = {.version = PW_VERSION_NODE, .info = PipeWireCore::node_info, .param = nullptr};

    PipeWireCore *pw = static_cast<PipeWireCore *>(data);
    if (qstrcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        pw_proxy_pointer<pw_node> n(static_cast<pw_node *>(pw_registry_bind(pw->pwRegistry.get(), id, type, PW_VERSION_NODE, 0)));
        PipeWireCore::Node node(id, std::move(n));
        pw_node_add_listener(node.node.get(), node.listener.get(), &pwNodeEvents, pw);
        pw->nodes.push_back(std::move(node));
    }
}

void PipeWireCore::registry_global_remove(void *data, uint32_t id)
{
    PipeWireCore *pw = static_cast<PipeWireCore *>(data);
    std::erase_if(pw->nodes, [id](const PipeWireCore::Node &node) {
        return node.id == id;
    });
}

void PipeWireCore::node_info(void *data, const struct pw_node_info *info)
{
    PipeWireCore *pw = static_cast<PipeWireCore *>(data);
    if (!(info->change_mask & PW_NODE_CHANGE_MASK_STATE)) {
        return;
    }
    auto node = std::ranges::find(pw->nodes, info->id, &PipeWireCore::Node::id);
    if (node == std::ranges::end(pw->nodes)) {
        return;
    }
    node->state = info->state;
    Q_EMIT pw->nodeStateChange(node->id, info->state);
}

bool PipeWireCore::isValid() const
{
    return m_valid;
}

std::optional<pw_node_state> PipeWireCore::getStreamState(uint32_t id)
{
    auto node = std::ranges::find(nodes, id, &PipeWireCore::Node::id);
    if (node == std::ranges::end(nodes)) {
        return {};
    }
    return node->state;
}

#include "moc_pipewirecore.cpp"
