// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick

/*
    \internal
    The purpose of this component is to be able to strongly type a dialog property.
    Specifically this allows PipeWireLayout to have a strongly typed property of
    the dialog while the dialog also has a strongly typed property of the
    layout. Without this intermediate component, we would have a circular dependency
    that QML cannot resolve.
*/
PortalDialog {
    property bool multiple: false

    signal clearSelection()
}
