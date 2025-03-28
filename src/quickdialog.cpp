/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "quickdialog.h"
#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

QuickDialog::QuickDialog(QObject *parent)
    : QObject(parent)
{
}

QuickDialog::~QuickDialog() noexcept
{
    delete m_theDialog;
}

void QuickDialog::create(const QString &file, const QVariantMap &props)
{
    auto engine = new QQmlApplicationEngine(this);
    auto context = new KLocalizedContext(engine);
    context->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(context);

    engine->setInitialProperties(props);
    engine->load(file);

    connect(engine, &QQmlEngine::warnings, this, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            qWarning() << warning;
        }
    });

    const QList<QObject *> rootObjects = engine->rootObjects();
    if (rootObjects.isEmpty()) {
        return;
    }
    m_theDialog = qobject_cast<QQuickWindow *>(rootObjects.constFirst());
    connect(m_theDialog, SIGNAL(accept()), this, SLOT(accept()));
    connect(m_theDialog, SIGNAL(reject()), this, SLOT(reject()));

    QTimer::singleShot(0, m_theDialog, SLOT(present()));
}

void QuickDialog::reject()
{
    Q_EMIT rejected();
    Q_EMIT finished(DialogResult::Rejected);
    deleteLater();
}

void QuickDialog::accept()
{
    Q_EMIT accepted();
    Q_EMIT finished(DialogResult::Accepted);
    deleteLater();
}
