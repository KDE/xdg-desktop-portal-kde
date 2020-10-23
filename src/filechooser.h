/*
 * Copyright © 2016-2018 Red Hat, Inc
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

#ifndef XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H
#define XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H

#include <QCheckBox>
#include <QComboBox>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QDialog>

class KFileWidget;
class QDialogButtonBox;
class MobileFileDialog;

class FileDialog : public QDialog
{
    Q_OBJECT
public:
    friend class FileChooserPortal;

    FileDialog(QDialog *parent = nullptr, Qt::WindowFlags flags = {});
    ~FileDialog();

private:
    QDialogButtonBox *m_buttons;
protected:
    KFileWidget *m_fileWidget;
};

class FileChooserPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.FileChooser")
public:
    // Keep in sync with qflatpakfiledialog from flatpak-platform-plugin
    typedef struct {
        uint type;
        QString filterString;
    } Filter;
    typedef QList<Filter> Filters;

    typedef struct {
        QString userVisibleName;
        Filters filters;
    } FilterList;
    typedef QList<FilterList> FilterListList;

    typedef struct {
        QString id;
        QString value;
    } Choice;
    typedef QList<Choice> Choices;

    typedef struct {
        QString id;
        QString label;
        Choices choices;
        QString initialChoiceId;
    } Option;
    typedef QList<Option> OptionList;

    explicit FileChooserPortal(QObject *parent);
    ~FileChooserPortal();

public Q_SLOTS:
    uint OpenFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  QVariantMap &results);

    uint SaveFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  QVariantMap &results);

private:
    static QWidget* CreateChoiceControls(const OptionList& optionList,
                                         QMap<QString, QCheckBox*>& checkboxes,
                                         QMap<QString, QComboBox*>& comboboxes);

    static QVariant EvaluateSelectedChoices(const QMap<QString, QCheckBox*>& checkboxes,
                                            const QMap<QString, QComboBox*>& comboboxes);

    static QString ExtractAcceptLabel(const QVariantMap &options);

    static void ExtractFilters(const QVariantMap &options, QStringList &nameFilters,
                               QStringList &mimeTypeFilters, QMap<QString, FilterList> &allFilters,
                               QString &selectedMimeTypeFilter);

    static bool isMobile();

    MobileFileDialog *m_mobileFileDialog;
};

#endif // XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H
