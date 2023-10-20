/*
    SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <qwayland-xdg-foreign-unstable-v2.h>

#include <QWaylandClientExtensionTemplate>

class XdgImported : public QtWayland::zxdg_imported_v2
{
public:
    XdgImported(::zxdg_imported_v2 *object);
    ~XdgImported() override;
};

class XdgImporter : public QWaylandClientExtensionTemplate<XdgImporter>, public QtWayland::zxdg_importer_v2
{
public:
    XdgImporter();
    ~XdgImporter() override;
    XdgImported *import(const QString &handle);
};
