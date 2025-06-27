/*
 * SPDX-FileCopyrightText: 2017 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 */

#include "accessdialog.h"
#include "accessdialog_debug.h"
#include "utils.h"

#include <KLocalizedString>
#include <QPushButton>
#include <QStandardPaths>
#include <QWindow>

using namespace Qt::StringLiterals;

using namespace Qt::StringLiterals;

AccessDialog::AccessDialog(QObject *parent)
    : QuickDialog(parent)
{
    m_props = {
        {u"iconName"_s, u"dialog-question"_s},
        {u"title"_s, i18n("Request device access")},
    };
}

void AccessDialog::setAcceptLabel(const QString &label)
{
    m_props.insert(QStringLiteral("acceptLabel"), label);
}

void AccessDialog::setBody(const QString &body)
{
    m_props.insert(QStringLiteral("body"), body);
}

void AccessDialog::setIcon(const QString &icon)
{
    m_props.insert(QStringLiteral("iconName"), icon);
}

void AccessDialog::setRejectLabel(const QString &label)
{
    m_props.insert(QStringLiteral("rejectLabel"), label);
}

void AccessDialog::setSubtitle(const QString &subtitle)
{
    m_props.insert(QStringLiteral("subtitle"), subtitle);
}

void AccessDialog::setTitle(const QString &title)
{
    m_props.insert(QStringLiteral("title"), title);
}

void AccessDialog::setChoices(const OptionList &choices)
{
    m_props.insert(u"choices"_s, QVariant::fromValue(choices));
}

Choices AccessDialog::selectedChoices() const
{
    auto props = m_theDialog->property("selectedChoices").value<QVariantMap>();
    Choices choices;
    choices.reserve(props.size());
    for (const auto &prop : props.asKeyValueRange()) {
        choices.emplaceBack(prop.first, prop.second.toString());
    }
    return choices;
}

void AccessDialog::createDialog()
{
    create(QStringLiteral("AccessDialog"), m_props);
}
