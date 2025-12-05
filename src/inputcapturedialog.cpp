/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#include "inputcapturedialog.h"

#include "utils.h"

#include <QWindow>

using namespace Qt::StringLiterals;

InputCaptureDialog::InputCaptureDialog(const QString &appId,
                                       InputCapturePortal::Capabilities capabilities,
                                       InputCapturePortal::PersistMode persistence,
                                       QObject *parent)
    : QuickDialog(parent)
{
    create(u"InputCaptureDialog"_s,
           {{u"app"_s, Utils::applicationName(appId)}, {u"persistenceRequested"_s, persistence != InputCapturePortal::PersistMode::None}});
}

bool InputCaptureDialog::allowRestore() const
{
    return m_theDialog->property("allowRestore").toBool();
}

#include "moc_inputcapturedialog.cpp"
