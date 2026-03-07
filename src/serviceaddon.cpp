// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include "serviceaddon.h"

#include <KService>

using namespace Qt::StringLiterals;

QString ServiceAddon::iconFromAppId(const QString &appId)
{
    auto service = KService::serviceByStorageId(appId);
    if (service && service->isValid()) {
        return service->icon();
    }
    return u"applications-other"_s;
}
