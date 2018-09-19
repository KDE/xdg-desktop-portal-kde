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

#include <QDBusMetaType>
#include <QDBusArgument>
#include <QLoggingCategory>
#include <QFileDialog>
#include <KLocalizedString>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeFileChooser, "xdp-kde-file-chooser")

// Keep in sync with qflatpakfiledialog from flatpak-platform-plugin
Q_DECLARE_METATYPE(FileChooserPortal::Filter)
Q_DECLARE_METATYPE(FileChooserPortal::Filters)
Q_DECLARE_METATYPE(FileChooserPortal::FilterList)
Q_DECLARE_METATYPE(FileChooserPortal::FilterListList)

QDBusArgument &operator << (QDBusArgument &arg, const FileChooserPortal::Filter &filter)
{
    arg.beginStructure();
    arg << filter.type << filter.filterString;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator >> (const QDBusArgument &arg, FileChooserPortal::Filter &filter)
{
    uint type;
    QString filterString;
    arg.beginStructure();
    arg >> type >> filterString;
    filter.type = type;
    filter.filterString = filterString;
    arg.endStructure();

    return arg;
}

QDBusArgument &operator << (QDBusArgument &arg, const FileChooserPortal::FilterList &filterList)
{
    arg.beginStructure();
    arg << filterList.userVisibleName << filterList.filters;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator >> (const QDBusArgument &arg, FileChooserPortal::FilterList &filterList)
{
    QString userVisibleName;
    FileChooserPortal::Filters filters;
    arg.beginStructure();
    arg >> userVisibleName >> filters;
    filterList.userVisibleName = userVisibleName;
    filterList.filters = filters;
    arg.endStructure();

    return arg;
}

FileChooserPortal::FileChooserPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<Filter>();
    qDBusRegisterMetaType<Filters>();
    qDBusRegisterMetaType<FilterList>();
    qDBusRegisterMetaType<FilterListList>();
}

FileChooserPortal::~FileChooserPortal()
{
}

uint FileChooserPortal::OpenFile(const QDBusObjectPath &handle,
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

    bool modalDialog = true;
    bool multipleFiles = false;
    QString acceptLabel;
    QStringList nameFilters;
    QStringList mimeTypeFilters;

    /* TODO
     * choices a(ssa(ss)s)
     * List of serialized combo boxes to add to the file chooser.
     *
     * For each element, the first string is an ID that will be returned with the response, te second string is a user-visible label.
     * The a(ss) is the list of choices, each being a is an ID and a user-visible label. The final string is the initial selection,
     * or "", to let the portal decide which choice will be initially selected. None of the strings, except for the initial selection, should be empty.
     *
     * As a special case, passing an empty array for the list of choices indicates a boolean choice that is typically displayed as a check button, using "true" and "false" as the choices.
     * Example: [('encoding', 'Encoding', [('utf8', 'Unicode (UTF-8)'), ('latin15', 'Western')], 'latin15'), ('reencode', 'Reencode', [], 'false')]
     */

    if (options.contains(QLatin1String("accept_label"))) {
        acceptLabel = options.value(QLatin1String("accept_label")).toString();
    }

    if (options.contains(QLatin1String("modal"))) {
        modalDialog = options.value(QLatin1String("modal")).toBool();
    }

    if (options.contains(QLatin1String("multiple"))) {
        multipleFiles = options.value(QLatin1String("multiple")).toBool();
    }

    if (options.contains(QLatin1String("filters"))) {
        FilterListList filterListList = qdbus_cast<FilterListList>(options.value(QLatin1String("filters")));
        Q_FOREACH (const FilterList &filterList, filterListList) {
            QStringList filterStrings;
            Q_FOREACH (const Filter &filterStruct, filterList.filters) {
                if (filterStruct.type == 0) {
                    filterStrings << filterStruct.filterString;
                } else {
                    mimeTypeFilters << filterStruct.filterString;
                }
            }

            if (!filterStrings.isEmpty()) {
                nameFilters << QStringLiteral("%1 (%2)").arg(filterList.userVisibleName).arg(filterStrings.join(QLatin1String(" ")));
            }
        }
    }

    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    fileDialog->setFileMode(multipleFiles ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);
    fileDialog->setLabelText(QFileDialog::Accept, !acceptLabel.isEmpty() ? acceptLabel : i18n("Open"));

    if (!nameFilters.isEmpty()) {
        fileDialog->setNameFilters(nameFilters);
    }

    if (!mimeTypeFilters.isEmpty()) {
        fileDialog->setMimeTypeFilters(mimeTypeFilters);
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

    fileDialog->deleteLater();
    return 1;
}

uint FileChooserPortal::SaveFile(const QDBusObjectPath &handle,
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

    bool modalDialog = true;
    QString acceptLabel;
    QString currentName;
    QString currentFolder;
    QString currentFile;
    QStringList nameFilters;
    QStringList mimeTypeFilters;

    // TODO parse options - choices

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
        currentFolder = QString::fromUtf8(options.value(QLatin1String("current_folder")).toByteArray());
    }

    if (options.contains(QLatin1String("current_file"))) {
        currentFile = QString::fromUtf8(options.value(QLatin1String("current_file")).toByteArray());
    }

    if (options.contains(QLatin1String("filters"))) {
        FilterListList filterListList = qdbus_cast<FilterListList>(options.value(QLatin1String("filters")));
        Q_FOREACH (const FilterList &filterList, filterListList) {
            QStringList filterStrings;
            Q_FOREACH (const Filter &filterStruct, filterList.filters) {
                if (filterStruct.type == 0) {
                    filterStrings << filterStruct.filterString;
                } else {
                    mimeTypeFilters << filterStruct.filterString;
                }
            }

            if (!filterStrings.isEmpty()) {
                nameFilters << QStringLiteral("%1 (%2)").arg(filterList.userVisibleName).arg(filterStrings.join(QLatin1String(" ")));
            }
        }
    }

    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);

    // TODO: Looks Qt doesn't have API for this
    // if (!currentName.isEmpty()) {
    //    fileDialog->selectFile(currentName);
    // }

    if (!currentFolder.isEmpty()) {
        fileDialog->setDirectoryUrl(QUrl(currentFolder));
    }

    if (!currentFile.isEmpty()) {
        fileDialog->selectFile(currentFile);
    }

    if (!acceptLabel.isEmpty()) {
        fileDialog->setLabelText(QFileDialog::Accept, acceptLabel);
    }

    if (!nameFilters.isEmpty()) {
        fileDialog->setNameFilters(nameFilters);
    }

    if (!mimeTypeFilters.isEmpty()) {
        fileDialog->setMimeTypeFilters(mimeTypeFilters);
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

    fileDialog->deleteLater();
    return 1;
}

