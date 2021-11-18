/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H

#include "screencast.h"
#include <QEventLoop>

class QWindow;

class ScreenChooserDialog : public QObject
{
    Q_OBJECT
public:
    ScreenChooserDialog(const QString &appName, bool multiple, ScreenCastPortal::SourceTypes types);
    ~ScreenChooserDialog() override;

    QList<quint32> selectedScreens() const;
    QVector<QMap<int, QVariant>> selectedWindows() const;
    QWindow *windowHandle() const
    {
        return m_theDialog;
    }

    bool exec();

public Q_SLOTS:
    void reject();
    void accept();

Q_SIGNALS:
    void clearSelection();

private:
    QWindow *m_theDialog = nullptr;
    QEventLoop m_execLoop;
};

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENCHOOSER_DIALOG_H
