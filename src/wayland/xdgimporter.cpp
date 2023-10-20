/*
    SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "xdgimporter.h"

XdgImported::XdgImported(::zxdg_imported_v2 *object)
    : QtWayland::zxdg_imported_v2(object)
{
}
XdgImported::~XdgImported()
{
    destroy();
}

XdgImporter::XdgImporter()
    : QWaylandClientExtensionTemplate(1)
{
}
XdgImporter::~XdgImporter()
{
    if (isActive()) {
        destroy();
    }
}

XdgImported *XdgImporter::import(const QString &handle)
{
    return new XdgImported(import_toplevel(handle));
}
