/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#include "inputcapturedialog.h"

#include "utils.h"

using namespace Qt::StringLiterals;

InputCaptureDialog::InputCaptureDialog(const QString &appId, [[maybe_unused]] InputCapturePortal::Capabilities capabilities, QObject *parent)
    : QuickDialog(parent)
{
    create(u"InputCaptureDialog"_s, {{u"app"_s, Utils::applicationName(appId)}});
}

#include "moc_inputcapturedialog.cpp"
