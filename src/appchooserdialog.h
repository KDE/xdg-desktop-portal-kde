/*
 * SPDX-FileCopyrightText: 2016-2019 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H

#include <QDialog>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

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
    Q_PROPERTY(bool showOnlyPreferredApps READ showOnlyPreferredApps WRITE setShowOnlyPrefferedApps NOTIFY showOnlyPreferredAppsChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit AppFilterModel(QObject *parent = nullptr);
    ~AppFilterModel() override;

    void setShowOnlyPrefferedApps(bool show);
    bool showOnlyPreferredApps() const;

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
};

class AppChooserData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QString defaultApp READ defaultApp WRITE setDefaultApp NOTIFY defaultAppChanged)

public:
    AppChooserData(QObject *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);
    Q_SIGNAL void fileNameChanged();

    QString defaultApp() const;
    void setDefaultApp(const QString &defaultApp);
    Q_SIGNAL void defaultAppChanged();

    Q_SIGNAL void applicationSelected(const QString &desktopFile);
    Q_SIGNAL void openDiscover();

private:
    QString m_defaultApp;
    QString m_fileName;
};

class AppModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ItemRoles {
        ApplicationNameRole = Qt::UserRole + 1,
        ApplicationIconRole,
        ApplicationDesktopFileRole,
        ApplicationCategoryRole,
    };

    explicit AppModel(QObject *parent = nullptr);
    ~AppModel() override;

    void setPreferredApps(const QStringList &list);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void loadApplications();

    QList<ApplicationItem> m_list;
};

class AppChooserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AppChooserDialog(const QStringList &choices,
                              const QString &defaultApp,
                              const QString &fileName,
                              const QString &mimeName = QString(),
                              QDialog *parent = nullptr,
                              Qt::WindowFlags flags = {});
    ~AppChooserDialog();

    void updateChoices(const QStringList &choices);

    QString selectedApplication() const;
private Q_SLOTS:
    void onApplicationSelected(const QString &desktopFile);
    void onOpenDiscover();

private:
    Ui::AppChooserDialog *m_dialog;

    AppModel *m_model;
    QStringList m_defaultChoices;
    QString m_defaultApp;
    QString m_selectedApplication;
    QString m_mimeName;
};

#endif // XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
