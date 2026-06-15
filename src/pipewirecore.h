/*
    SPDX-FileCopyrightText: 2018-2020 Red Hat Inc
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
    SPDX-FileContributor: Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDebug>
#include <QObject>

#include <pipewire/pipewire.h>
#include <spa/utils/hook.h>

struct ProxyDestroy {
    static void operator()(auto *p)
    {
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(p));
    }
};

template<typename T, auto destroy>
using pw_pointer = std::unique_ptr<T, std::integral_constant<decltype(destroy), destroy>>;
template<typename T>
using pw_proxy_pointer = std::unique_ptr<T, ProxyDestroy>;

class PipeWireCore : public QObject
{
    Q_OBJECT

public:
    PipeWireCore();

    ~PipeWireCore();

    bool isValid() const;
    std::optional<pw_node_state> getStreamState(uint32_t id);

Q_SIGNALS:
    void nodeStateChange(uint32_t id, pw_node_state state);

private:
    bool m_valid = false;
    struct Node {
        uint32_t id;
        pw_proxy_pointer<pw_node> node;
        std::optional<pw_node_state> state = {};
        std::unique_ptr<spa_hook> listener = std::make_unique<spa_hook>();
    };
    std::vector<Node> nodes;

    pw_pointer<pw_core, pw_core_disconnect> pwCore;
    pw_pointer<pw_context, pw_context_destroy> pwContext;
    pw_pointer<pw_loop, pw_loop_destroy> pwMainLoop;
    pw_proxy_pointer<pw_registry> pwRegistry;
    spa_hook coreListener;
    spa_hook registryListener;

    static void core_error(void *data, uint32_t id, int seq, int res, const char *message);
    static void registry_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props);
    static void registry_global_remove(void *data, uint32_t id);
    static void node_info(void *data, const struct pw_node_info *info);
};
