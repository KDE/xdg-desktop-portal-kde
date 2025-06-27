/*
 * SPDX-FileCopyrightText: 2016-2019 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 */

#ifndef XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
#define XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H

#include "quickdialog.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <qmimetype.h>

#include <KCompletion>
#include <KConfigGroup>
#include <KService>
#include <KSharedConfig>

#include "filechooser.h"
#include <KFileFilterCombo>
#include <KFileWidget>
#include <KProcess>
#include <QUrl>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

class ApplicationItem
{
public:
    enum ApplicationCategory {
        PreferredApplication,
        AllApplications,
    };

    explicit ApplicationItem(const QString &name, const KService::Ptr &service);

    QString applicationName() const;
    QString applicationGenericName() const;
    QString applicationUntranslatedGenericName() const;
    QString applicationIcon() const;
    QString applicationDesktopFile() const;
    QList<QMimeType> supportedMimeTypes() const;

    void setApplicationCategory(ApplicationCategory category);
    ApplicationCategory applicationCategory() const;

    bool operator==(const ApplicationItem &item) const;

private:
    QString m_applicationName;
    KService::Ptr m_applicationService;
    QList<QMimeType> m_supportedMimeTypes;
    ApplicationCategory m_applicationCategory;
};

class AppFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by C++ side via properties")
    Q_PROPERTY(bool showOnlyPreferredApps READ showOnlyPreferredApps WRITE setShowOnlyPreferredApps NOTIFY showOnlyPreferredAppsChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    explicit AppFilterModel(QObject *parent = nullptr);

    void setShowOnlyPreferredApps(bool show);
    bool showOnlyPreferredApps() const;

    void setDefaultApp(const QString &);
    QString defaultApp() const;

    void setLastUsedApp(const QString &);
    QString lastUsedApp() const;

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
    QString m_lastUsedApp;
};

class AppChooserData : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by C++ side via properties")

    // Outgoing to QML
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QString defaultApp READ defaultApp WRITE setDefaultApp NOTIFY defaultAppChanged)
    Q_PROPERTY(QString lastUsedApp READ lastUsedApp WRITE setLastUsedApp NOTIFY lastUsedAppChanged)
    Q_PROPERTY(QString mimeName READ mimeName WRITE setMimeName NOTIFY mimeNameChanged)
    Q_PROPERTY(QString mimeDesc READ mimeDesc NOTIFY mimeDescChanged)
    Q_PROPERTY(QStringList history MEMBER m_history NOTIFY historyChanged)
    Q_PROPERTY(bool shellAccess MEMBER m_shellAccess NOTIFY shellAccessChanged)

    // Incoming from QML
    Q_PROPERTY(bool remember MEMBER m_remember NOTIFY rememberChanged)
    Q_PROPERTY(bool openInTerminal MEMBER m_openInTerminal NOTIFY openInTerminalChanged)
    Q_PROPERTY(bool lingerTerminal MEMBER m_lingerTerminal NOTIFY lingerTerminalChanged)
public:
    explicit AppChooserData(QObject *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);
    Q_SIGNAL void fileNameChanged();

    QString defaultApp() const;
    void setDefaultApp(const QString &defaultApp);
    Q_SIGNAL void defaultAppChanged();

    QString lastUsedApp() const;
    void setLastUsedApp(const QString &lastUsedApp);
    Q_SIGNAL void lastUsedAppChanged();

    QString mimeName() const;
    void setMimeName(const QString &mimeName);
    Q_SIGNAL void mimeNameChanged();

    QString mimeDesc() const;
    void setMimeDesc(const QString &mimeDesc);
    Q_SIGNAL void mimeDescChanged();

    void setHistory(const QStringList &history);
    Q_SIGNAL void historyChanged();

    void setShellAccess(bool enable);
    Q_SIGNAL void shellAccessChanged();

    bool m_remember = false;
    Q_SIGNAL void rememberChanged();

    bool m_openInTerminal = false;
    Q_SIGNAL void openInTerminalChanged();

    bool m_lingerTerminal = false;
    Q_SIGNAL void lingerTerminalChanged();

    Q_SIGNAL void applicationSelected(const QString &desktopFile, bool remember = false);
    Q_SIGNAL void openDiscover();

    Q_SIGNAL void fileDialogFinished(const QString &selectedFile);

public Q_SLOTS:
    void openFileDialog(QWindow *parent = nullptr)
    {
        auto fileDialog = new FileDialog;
        fileDialog->winId(); // so it creates windowHandle
        fileDialog->windowHandle()->setTransientParent(parent);
        fileDialog->setWindowModality(Qt::WindowModal);
        fileDialog->m_fileWidget->setMode(KFile::Mode::File | KFile::Mode::ExistingOnly);
        fileDialog->m_fileWidget->setSupportedSchemes({QStringLiteral("file")});
        connect(fileDialog, &QDialog::finished, this, [this, fileDialog](int result) {
            fileDialog->deleteLater();
            if (result != QDialog::Accepted) {
                Q_EMIT fileDialogFinished({});
                return;
            }
            const auto urls = fileDialog->m_fileWidget->selectedUrls();
            Q_EMIT fileDialogFinished(urls.at(0).toLocalFile());
        });
        fileDialog->open();
    }

private:
    QString m_defaultApp;
    QString m_lastUsedApp;
    QString m_fileName;
    QString m_mimeName;
    QStringList m_history;
    bool m_shellAccess = false;
};

class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasPreferredApps MEMBER m_hasPreferredApps NOTIFY hasPreferredAppsChanged)
public:
    enum ItemRoles {
        ApplicationNameRole = Qt::UserRole + 1,
        ApplicationGenericNameRole,
        ApplicationUntranslatedGenericNameRole,
        ApplicationIconRole,
        ApplicationDesktopFileRole,
        ApplicationCategoryRole,
        ApplicationSupportedMimeTypesRole,
    };

    explicit AppModel(QObject *parent = nullptr);

    // NOTE This invalidates preferred apps, call setPreferredApps aftetwards!
    void loadApplications();

    void setPreferredApps(const QStringList &list);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void hasPreferredAppsChanged();

private:
    QList<ApplicationItem> m_list;
    QHash<QString, QString> m_noDisplayAliasesFor;
    bool m_hasPreferredApps = false;
};

class AppChooserDialog : public QuickDialog
{
    Q_OBJECT
public:
    explicit AppChooserDialog(const QStringList &choices, const QString &lastUsedApp, const QString &fileName, const QString &mimeName, bool autoRemember);
    void updateChoices(const QStringList &choices);

    QString selectedApplication() const;
    QString activationToken() const;
public Q_SLOTS:
    void onApplicationSelected(const QString &desktopFile, const bool remember = false);
    void onOpenDiscover();

public:
    AppModel *const m_model;
    QString m_selectedApplication;
    QString m_activationToken;
    AppChooserData *const m_appChooserData;
    bool m_autoRemember;
};

#endif // XDG_DESKTOP_PORTAL_KDE_APPCHOOSER_DIALOG_H
