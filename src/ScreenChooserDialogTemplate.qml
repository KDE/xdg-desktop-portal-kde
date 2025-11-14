// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

pragma ComponentBehavior: Bound

import QtQuick

PortalDialog {
    property bool multiple: false

    signal clearSelection()
}
