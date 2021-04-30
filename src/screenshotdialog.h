/*
 * SPDX-FileCopyrightText: 2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_SCREENSHOT_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_SCREENSHOT_DIALOG_H

#include <QDialog>
#include <QFuture>

namespace Ui
{
class ScreenshotDialog;
}

class ScreenshotDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScreenshotDialog(QDialog *parent = nullptr, Qt::WindowFlags flags = {});
    ~ScreenshotDialog();

    QImage image() const;

    void takeScreenshotNonInteractive();

private Q_SLOTS:
    void takeScreenshotInteractive();

Q_SIGNALS:
    void failed();

private:
    Ui::ScreenshotDialog *m_dialog;
    QImage m_image;

    int mask();
    QFuture<QImage> takeScreenshot();
};

#endif // XDG_DESKTOP_PORTAL_KDE_SCREENSHOT_DIALOG_H
