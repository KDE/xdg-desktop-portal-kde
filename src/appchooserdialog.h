/*
 * SPDX-FileCopyrightText: 2016-2019 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H

#include "quickdialog.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <qmimetype.h>

namespace Ui
{
class AppChooserDialog;
}

class ApplicationItem
{
public:
    enum ApplicationCategory {
        PreferredApplication,
        AllApplications,
    };

    explicit ApplicationItem(const QString &name, const QString &icon, const QString &desktopFileName);

    QString applicationName() const;
    QString applicationIcon() const;
    QString applicationDesktopFile() const;

    void setApplicationCategory(ApplicationCategory category);
    ApplicationCategory applicationCategory() const;

    bool operator==(const ApplicationItem &item) const;

private:
    QString m_applicationName;
    QString m_applicationIcon;
    QString m_applicationDesktopFile;
    ApplicationCategory m_applicationCategory;
};

class AppFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool showOnlyPreferredApps READ showOnlyPreferredApps WRITE setShowOnlyPreferredApps NOTIFY showOnlyPreferredAppsChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit AppFilterModel(QObject *parent = nullptr);

    void setShowOnlyPreferredApps(bool show);
    bool showOnlyPreferredApps() const;

    void setDefaultApp(const QString &);
    QString defaultApp() const;

    void setFilter(const QString &text);
    QString filter() const;

Q_SIGNALS:
    void showOnlyPreferredAppsChanged();
    void filterChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool m_showOnlyPreferredApps = true;
    QString m_filter;
    QString m_defaultApp;
};

class AppChooserData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QString defaultApp READ defaultApp WRITE setDefaultApp NOTIFY defaultAppChanged)
    Q_PROPERTY(QString mimeName READ mimeName WRITE setMimeName NOTIFY mimeNameChanged)
    Q_PROPERTY(QString mimeDesc READ mimeDesc NOTIFY mimeDescChanged)

public:
    explicit AppChooserData(QObject *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);
    Q_SIGNAL void fileNameChanged();

    QString defaultApp() const;
    void setDefaultApp(const QString &defaultApp);
    Q_SIGNAL void defaultAppChanged();

    QString mimeName() const;
    void setMimeName(const QString &mimeName);
    Q_SIGNAL void mimeNameChanged();

    QString mimeDesc() const;
    void setMimeDesc(const QString &mimeDesc);
    Q_SIGNAL void mimeDescChanged();

    Q_SIGNAL void applicationSelected(const QString &desktopFile, bool remember = false);
    Q_SIGNAL void openDiscover();

private:
    QString m_defaultApp;
    QString m_fileName;
    QString m_mimeName;
};

class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasPreferredApps MEMBER m_hasPreferredApps NOTIFY hasPreferredAppsChanged)
public:
    enum ItemRoles {
        ApplicationNameRole = Qt::UserRole + 1,
        ApplicationIconRole,
        ApplicationDesktopFileRole,
        ApplicationCategoryRole,
    };

    explicit AppModel(QObject *parent = nullptr);

    void setPreferredApps(const QStringList &list);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void hasPreferredAppsChanged();

private:
    void loadApplications();

    QList<ApplicationItem> m_list;
    QHash<QString, QString> m_noDisplayAliasesFor;
    bool m_hasPreferredApps = false;
};

class AppChooserDialog : public QuickDialog
{
    Q_OBJECT
public:
    explicit AppChooserDialog(const QStringList &choices,
                              const QString &defaultApp,
                              const QString &fileName,
                              const QString &mimeName = QString(),
                              QObject *parent = nullptr);
    void updateChoices(const QStringList &choices);

    QString selectedApplication() const;
private Q_SLOTS:
    void onApplicationSelected(const QString &desktopFile, const bool remember = false);
    void onOpenDiscover();

private:
    AppModel *const m_model;
    QString m_selectedApplication;
    AppChooserData *const m_appChooserData;
};

#endif // XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
