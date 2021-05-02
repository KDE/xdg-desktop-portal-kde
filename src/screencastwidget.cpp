/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#include "screencastwidget.h"
#include "waylandintegration.h"

#include <KLocalizedString>

ScreenCastWidget::ScreenCastWidget(QWidget *parent)
    : QListWidget(parent)
{
    QMapIterator<quint32, WaylandIntegration::WaylandOutput> it(WaylandIntegration::screens());
    while (it.hasNext()) {
        it.next();
        QListWidgetItem *widgetItem = new QListWidgetItem(this);
        widgetItem->setData(Qt::UserRole, it.key());
        if (it.value().outputType() == WaylandIntegration::WaylandOutput::Laptop) {
            widgetItem->setIcon(QIcon::fromTheme(QStringLiteral("computer-laptop")));
            widgetItem->setText(i18n("Laptop screen\nModel: %1", it.value().model()));
        } else if (it.value().outputType() == WaylandIntegration::WaylandOutput::Monitor) {
            widgetItem->setIcon(QIcon::fromTheme(QStringLiteral("video-display")));
            widgetItem->setText(i18n("Manufacturer: %1\nModel: %2", it.value().manufacturer(), it.value().model()));
        } else {
            widgetItem->setIcon(QIcon::fromTheme(QStringLiteral("video-television")));
            widgetItem->setText(i18n("Manufacturer: %1\nModel: %2", it.value().manufacturer(), it.value().model()));
        }
    }
}

ScreenCastWidget::~ScreenCastWidget()
{
}

QList<quint32> ScreenCastWidget::selectedScreens() const
{
    QList<quint32> selectedScreens;

    const auto selectedItems = this->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        selectedScreens << item->data(Qt::UserRole).toUInt();
    }

    return selectedScreens;
}
