/*
 * SPDX-FileCopyrightText: 2017-2019 Red Hat Inc
 * SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText:  2017-2019 Jan Grulich <jgrulich@redhat.com>
 */

#include "appchooserdialog.h"
#include "appchooser_debug.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>

#include <QApplication>
#include <QDir>
#include <QMimeDatabase>
#include <QSettings>
#include <QStandardPaths>

#include <KApplicationTrader>
#include <KIO/MimeTypeFinderJob>
#include <KLocalizedString>
#include <KProcess>
#include <KService>
#include <algorithm>
#include <iterator>
#include <KBuildSycocaProgressDialog>
#include <kapplicationtrader.h>

AppChooserDialog::AppChooserDialog(const QStringList &choices, const QString &defaultApp, const QString &fileName, const QString &mimeName, QObject *parent)
    : QuickDialog(parent)
    , m_model(new AppModel(this))
    , m_appChooserData(new AppChooserData(this))
{
    QVariantMap props = {
        {"title", i18nc("@title:window", "Choose Application")},
        // fileName is actually the full path, confusingly enough. But showing the
        // whole thing is overkill; let's just show the user the file itself
        {"mainText", xi18nc("@info", "Choose an application to open <filename>%1</filename>", QUrl::fromLocalFile(fileName).fileName())},
    };

    AppFilterModel *filterModel = new AppFilterModel(this);
    filterModel->setSourceModel(m_model);

    m_appChooserData->setFileName(fileName);
    m_appChooserData->setDefaultApp(defaultApp);
    filterModel->setDefaultApp(defaultApp);

    auto findDefaultApp = [this, defaultApp, filterModel]() {
        if (!defaultApp.isEmpty()) {
            return;
        }
        KService::Ptr defaultService = KApplicationTrader::preferredService(m_appChooserData->mimeName());
        if (defaultService && defaultService->isValid()) {
            QString id = defaultService->desktopEntryName();
            m_appChooserData->setDefaultApp(id);
            filterModel->setDefaultApp(id);
        }
    };

    auto findPreferredApps = [this, choices]() {
        if (!choices.isEmpty()) {
            m_model->setPreferredApps(choices);
            return;
        }
        QStringList choices;
        const KService::List appServices = KApplicationTrader::queryByMimeType(m_appChooserData->mimeName(), [](const KService::Ptr &service) -> bool {
            return service->isValid();
        });
        std::transform(appServices.begin(), appServices.end(), std::back_inserter(choices), [](const KService::Ptr &service) {
            return service ? service->desktopEntryName() : QString();
        });
        m_model->setPreferredApps(choices);
    };

    if (mimeName.isEmpty()) {
        auto job = new KIO::MimeTypeFinderJob(QUrl::fromUserInput(fileName));
        job->setAuthenticationPromptEnabled(false);
        connect(job, &KIO::MimeTypeFinderJob::result, this, [this, job, findDefaultApp, findPreferredApps]() {
            if (job->error() == KJob::NoError) {
                m_appChooserData->setMimeName(job->mimeType());
                findDefaultApp();
                findPreferredApps();
            } else {
                qCWarning(XdgDesktopPortalKdeAppChooser) << "couldn't get mimetype:" << job->errorString();
            }
        });
        job->start();
    } else {
        m_appChooserData->setMimeName(mimeName);
        findDefaultApp();
        findPreferredApps();
    }

    qmlRegisterSingletonInstance<AppFilterModel>("org.kde.xdgdesktopportal", 1, 0, "AppModel", filterModel);
    qmlRegisterSingletonInstance<AppChooserData>("org.kde.xdgdesktopportal", 1, 0, "AppChooserData", m_appChooserData);

    create(QStringLiteral("qrc:/AppChooserDialog.qml"), props);

    connect(m_appChooserData, &AppChooserData::openDiscover, this, &AppChooserDialog::onOpenDiscover);
    connect(m_appChooserData, &AppChooserData::applicationSelected, this, &AppChooserDialog::onApplicationSelected);
}

QString AppChooserDialog::selectedApplication() const
{
    return m_selectedApplication;
}

void AppChooserDialog::onApplicationSelected(const QString &desktopFile, const bool remember)
{
    m_selectedApplication = desktopFile;

    if (remember && !m_appChooserData->mimeName().isEmpty()) {
        KService::Ptr serv = KService::serviceByDesktopName(desktopFile);
        KApplicationTrader::setPreferredService(m_appChooserData->mimeName(), serv);
        // kbuildsycoca is the one reading mimeapps.list, so we need to run it now
        KBuildSycocaProgressDialog::rebuildKSycoca(QApplication::activeWindow());
    }

    accept();
}

void AppChooserDialog::onOpenDiscover()
{
    QStringList args;
    if (!m_appChooserData->mimeName().isEmpty()) {
        args << QStringLiteral("--mime") << m_appChooserData->mimeName();
    }
    KProcess::startDetached(QStringLiteral("plasma-discover"), args);
}

void AppChooserDialog::updateChoices(const QStringList &choices)
{
    m_model->setPreferredApps(choices);
}

ApplicationItem::ApplicationItem(const QString &name, const QString &icon, const QString &desktopFileName)
    : m_applicationName(name)
    , m_applicationIcon(icon)
    , m_applicationDesktopFile(desktopFileName)
    , m_applicationCategory(AllApplications)
{
}

QString ApplicationItem::applicationName() const
{
    return m_applicationName;
}

QString ApplicationItem::applicationIcon() const
{
    return m_applicationIcon;
}

QString ApplicationItem::applicationDesktopFile() const
{
    return m_applicationDesktopFile;
}

void ApplicationItem::setApplicationCategory(ApplicationItem::ApplicationCategory category)
{
    m_applicationCategory = category;
}

ApplicationItem::ApplicationCategory ApplicationItem::applicationCategory() const
{
    return m_applicationCategory;
}

bool ApplicationItem::operator==(const ApplicationItem &item) const
{
    return item.applicationDesktopFile() == applicationDesktopFile();
}

AppFilterModel::AppFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    sort(0, Qt::DescendingOrder);
}

AppFilterModel::~AppFilterModel()
{
}

void AppFilterModel::setShowOnlyPrefferedApps(bool show)
{
    m_showOnlyPreferredApps = show;

    invalidate();
}

bool AppFilterModel::showOnlyPreferredApps() const
{
    return m_showOnlyPreferredApps;
}

void AppFilterModel::setDefaultApp(const QString &defaultApp)
{
    m_defaultApp = defaultApp;

    invalidate();
}

QString AppFilterModel::defaultApp() const
{
    return m_defaultApp;
}

void AppFilterModel::setFilter(const QString &text)
{
    m_filter = text;

    invalidate();
}

QString AppFilterModel::filter() const
{
    return m_filter;
}

bool AppFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    ApplicationItem::ApplicationCategory category =
        static_cast<ApplicationItem::ApplicationCategory>(sourceModel()->data(index, AppModel::ApplicationCategoryRole).toInt());
    QString appName = sourceModel()->data(index, AppModel::ApplicationNameRole).toString();

    if (m_filter.isEmpty()) {
        return m_showOnlyPreferredApps ? category == ApplicationItem::PreferredApplication : true;
    }

    return m_showOnlyPreferredApps ? category == ApplicationItem::PreferredApplication && appName.contains(m_filter, Qt::CaseInsensitive)
                                   : appName.contains(m_filter, Qt::CaseInsensitive);
}

bool AppFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (sourceModel()->data(left, AppModel::ApplicationDesktopFileRole) == m_defaultApp) {
        return false;
    }
    if (sourceModel()->data(right, AppModel::ApplicationDesktopFileRole) == m_defaultApp) {
        return true;
    }
    ApplicationItem::ApplicationCategory leftCategory =
        static_cast<ApplicationItem::ApplicationCategory>(sourceModel()->data(left, AppModel::ApplicationCategoryRole).toInt());
    ApplicationItem::ApplicationCategory rightCategory =
        static_cast<ApplicationItem::ApplicationCategory>(sourceModel()->data(right, AppModel::ApplicationCategoryRole).toInt());
    QString leftName = sourceModel()->data(left, AppModel::ApplicationNameRole).toString();
    QString rightName = sourceModel()->data(right, AppModel::ApplicationNameRole).toString();

    if (leftCategory < rightCategory) {
        return false;
    } else if (leftCategory > rightCategory) {
        return true;
    }

    return QString::localeAwareCompare(leftName, rightName) > 0;
}

QString AppChooserData::defaultApp() const
{
    return m_defaultApp;
}

void AppChooserData::setDefaultApp(const QString &defaultApp)
{
    m_defaultApp = defaultApp;
    Q_EMIT defaultAppChanged();
}

AppChooserData::AppChooserData(QObject *parent)
    : QObject(parent)
{
}

QString AppChooserData::fileName() const
{
    return m_fileName;
}

void AppChooserData::setFileName(const QString &fileName)
{
    m_fileName = fileName;
    Q_EMIT fileNameChanged();
}

QString AppChooserData::mimeName() const
{
    return m_mimeName;
}

void AppChooserData::setMimeName(const QString &mimeName)
{
    if (m_mimeName != mimeName) {
        m_mimeName = mimeName;
        Q_EMIT mimeNameChanged();
        Q_EMIT mimeDescChanged();
    }
}

QString AppChooserData::mimeDesc() const
{
    return QMimeDatabase().mimeTypeForName(m_mimeName).comment();
}

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadApplications();
}

AppModel::~AppModel()
{
}

void AppModel::setPreferredApps(const QStringList &possiblyAliasedList)
{
    m_hasPreferredApps = false;
    Q_EMIT hasPreferredAppsChanged();

    // In the event that we get incoming NoDisplay entries that are AliasFor another desktop file,
    // switch the NoDisplay name for the aliased name.
    QStringList list;
    for (const auto &entry : possiblyAliasedList) {
        if (const auto value = m_noDisplayAliasesFor.value(entry); !value.isEmpty()) {
            list << value;
        } else {
            list << entry;
        }
    }

    for (ApplicationItem &item : m_list) {
        bool changed = false;

        // First reset to initial type
        if (item.applicationCategory() != ApplicationItem::AllApplications) {
            item.setApplicationCategory(ApplicationItem::AllApplications);
            changed = true;
        }

        if (list.contains(item.applicationDesktopFile())) {
            item.setApplicationCategory(ApplicationItem::PreferredApplication);
            changed = true;
            m_hasPreferredApps = true;
            Q_EMIT hasPreferredAppsChanged();
        }

        if (changed) {
            const int row = m_list.indexOf(item);
            if (row >= 0) {
                QModelIndex index = createIndex(row, 0, AppModel::ApplicationCategoryRole);
                Q_EMIT dataChanged(index, index);
            }
        }
    }
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();

    if (row >= 0 && row < m_list.count()) {
        ApplicationItem item = m_list.at(row);

        switch (role) {
        case ApplicationNameRole:
            return item.applicationName();
        case ApplicationIconRole:
            return item.applicationIcon();
        case ApplicationDesktopFileRole:
            return item.applicationDesktopFile();
        case ApplicationCategoryRole:
            return static_cast<int>(item.applicationCategory());
        default:
            break;
        }
    }

    return QVariant();
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_list.count();
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        {ApplicationNameRole, QByteArrayLiteral("applicationName")},
        {ApplicationIconRole, QByteArrayLiteral("applicationIcon")},
        {ApplicationDesktopFileRole, QByteArrayLiteral("applicationDesktopFile")},
        {ApplicationCategoryRole, QByteArrayLiteral("applicationCategory")},
    };
}

void AppModel::loadApplications()
{
    const KService::List appServices = KApplicationTrader::query([](const KService::Ptr &service) -> bool {
        return service->isValid();
    });
    for (const KService::Ptr &service : appServices) {
        if (service->noDisplay()) {
            if (const auto alias = service->aliasFor(); !alias.isEmpty()) {
                m_noDisplayAliasesFor.insert(service->desktopEntryName(), service->aliasFor());
            }
            continue; // no display after all
        }

        const QString fullName = service->property(QStringLiteral("X-GNOME-FullName"), QMetaType::QString).toString();
        const QString name = fullName.isEmpty() ? service->name() : fullName;
        ApplicationItem appItem(name, service->icon(), service->desktopEntryName());

        if (!m_list.contains(appItem)) {
            m_list.append(appItem);
        }
    }
}
