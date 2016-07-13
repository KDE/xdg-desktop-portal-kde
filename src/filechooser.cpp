/*
 * Copyright © 2016 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "filechooser.h"

#include <QLoggingCategory>
#include <QFileDialog>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeFileChooser, "xdg-desktop-portal-kde-file-chooser")


FileChooser::FileChooser(QObject *parent)
    : QObject(parent)
{
}

FileChooser::~FileChooser()
{
}

uint FileChooser::OpenFile(const QDBusObjectPath &handle,
                           const QString &app_id,
                           const QString &parent_window,
                           const QString &title,
                           const QVariantMap &options,
                           QVariantMap &results)
{
    Q_UNUSED(app_id);

    qCDebug(XdgDesktopPortalKdeFileChooser) << "OpenFile called with parameters:";
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    options: " << options;

    QString acceptLabel;
    bool modalDialog = true;
    bool multipleFiles = false;

    // TODO parse options - filters, choices

    if (options.contains(QLatin1String("multiple"))) {
        multipleFiles = options.value(QLatin1String("multiple")).toBool();
    }

    if (options.contains(QLatin1String("modal"))) {
        modalDialog = options.value(QLatin1String("modal")).toBool();
    }

    if (options.contains(QLatin1String("accept_label"))) {
        acceptLabel = options.value(QLatin1String("accept_label")).toString();
    }

    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    fileDialog->setFileMode(multipleFiles ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);

    if (!acceptLabel.isEmpty()) {
        fileDialog->setLabelText(QFileDialog::Accept, acceptLabel);
    }

    if (fileDialog->exec() == QDialog::Accepted) {
        QStringList files;
        Q_FOREACH (const QString &filename, fileDialog->selectedFiles()) {
           QUrl url = QUrl::fromLocalFile(filename);
           files << url.toDisplayString();
        }
        results.insert(QLatin1String("uris"), files);
        fileDialog->deleteLater();
        return 0;
    }

    return 1;
}

uint FileChooser::SaveFile(const QDBusObjectPath &handle,
                           const QString &app_id,
                           const QString &parent_window,
                           const QString &title,
                           const QVariantMap &options,
                           QVariantMap &results)
{
    Q_UNUSED(app_id);

    qCDebug(XdgDesktopPortalKdeFileChooser) << "SaveFile called with parameters:";
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdeFileChooser) << "    options: " << options;

    QString acceptLabel;
    QString currentName;
    QString currentFolder;
    QString currentFile;

    bool modalDialog = true;

    // TODO parse options - filters, choices

    if (options.contains(QLatin1String("modal"))) {
        modalDialog = options.value(QLatin1String("modal")).toBool();
    }

    if (options.contains(QLatin1String("accept_label"))) {
        acceptLabel = options.value(QLatin1String("accept_label")).toString();
    }

    if (options.contains(QLatin1String("current_name"))) {
        currentName = options.value(QLatin1String("current_name")).toString();
    }

    if (options.contains(QLatin1String("current_folder"))) {
        currentFolder = options.value(QLatin1String("current_folder")).toString();
    }

    if (options.contains(QLatin1String("current_file"))) {
        currentFile = options.value(QLatin1String("current_file")).toString();
    }

    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);

    if (!currentName.isEmpty()) {
        fileDialog->selectFile(currentName);
    }

    if (!currentFolder.isEmpty()) {
        fileDialog->setDirectory(currentFolder);
    }

    if (!currentFile.isEmpty()) {
        fileDialog->selectFile(currentName);
    }

    if (!acceptLabel.isEmpty()) {
        fileDialog->setLabelText(QFileDialog::Accept, acceptLabel);
    }

    if (fileDialog->exec() == QDialog::Accepted) {
        QStringList files;
        Q_FOREACH (const QString &filename, fileDialog->selectedFiles()) {
           QUrl url = QUrl::fromLocalFile(filename);
           files << url.toDisplayString();
        }
        results.insert(QLatin1String("uris"), files);
        fileDialog->deleteLater();
        return 0;
    }

    return 1;
}

