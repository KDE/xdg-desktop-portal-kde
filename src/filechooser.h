/*
 * SPDX-FileCopyrightText: 2016-2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H
#define XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H

#include "dbushelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDialog>

#include <KConfigGroup>
#include <KFileFilter>

class KFileWidget;
class QDialogButtonBox;
class MobileFileDialog;
class QDBusMessage;

class FileDialog : public QDialog
{
    Q_OBJECT
public:
    friend class FileChooserPortal;
    friend class AppModel;

    explicit FileDialog(QDialog *parent = nullptr, Qt::WindowFlags flags = {});
    ~FileDialog() override;
    Q_DISABLE_COPY_MOVE(FileDialog)

public:
    KFileWidget *const m_fileWidget;
    KConfigGroup m_configGroup;
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

    explicit FileChooserPortal(QObject *parent);

public Q_SLOTS:
    void OpenFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  const QDBusMessage &message,
                  uint &replyResponse,
                  QVariantMap &replyResults);

    void SaveFile(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QVariantMap &options,
                  const QDBusMessage &message,
                  uint &replyResponse,
                  QVariantMap &replyResults);

private:
    static QWidget *CreateChoiceControls(const OptionList &optionList, QMap<QString, QCheckBox *> &checkboxes, QMap<QString, QComboBox *> &comboboxes);

    static QVariant EvaluateSelectedChoices(const QMap<QString, QCheckBox *> &checkboxes, const QMap<QString, QComboBox *> &comboboxes);

    static QString ExtractAcceptLabel(const QVariantMap &options);

    struct FilterExtract {
        QVector<KFileFilter> filters;
        KFileFilter currentFilter;
    };

    static FilterExtract ExtractFilters(const QVariantMap &options);

    static bool isMobile();

    static FilterList fileFilterToFilterList(const KFileFilter &filter);
    static KFileFilter filterListToFileFilter(const FilterList &list);

    MobileFileDialog *m_mobileFileDialog = nullptr;
};

#endif // XDG_DESKTOP_PORTAL_KDE_FILECHOOSER_H
