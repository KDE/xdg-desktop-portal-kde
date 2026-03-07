// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class ServiceAddon : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    using QObject::QObject;

public Q_SLOTS:
    [[nodiscard]] QString iconFromAppId(const QString &appId);
};
