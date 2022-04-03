// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "dirmodel.h"

#include <QDebug>
#include <QSize>

#include <KDirLister>
#include <KIO/Job>
#include <KIO/PreviewJob>

#include "dirlister.h"

DirModel::DirModel(QObject *parent)
    : KDirSortFilterProxyModel(parent)
    , m_lister(new KDirLister(this))
{
    setSourceModel(&m_dirModel);
    m_dirModel.setDirLister(m_lister);
    connect(&m_dirModel, &QAbstractListModel::rowsInserted, [this](const QModelIndex &parent, int first, int last) {
        Q_UNUSED(parent);

        KFileItemList items;
        items.reserve(last - first);

        // Fetch file items from all affected rows
        for (int i = first; i < last; i++) {
            auto item = qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index(i, 0), KDirModel::FileItemRole));
            items.append(item);
        }
        auto *job = new KIO::PreviewJob(items, m_thumbnailSize);
        connect(job, &KIO::PreviewJob::gotPreview, this, [this, first, items](const KFileItem &item, const QPixmap &preview) {
            m_previews.insert(item, preview);

            // TODO check if n * indexOf is actually better than n * dataChanged(first, last);
            int i = items.indexOf(item);
            if (i == -1) {
                return;
            }

            int row = i + first;
            auto modelIndex = index(row, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex, {Roles::Thumbnail});
        });
    });
    connect(m_lister, QOverload<>::of(&KCoreDirLister::completed), this, &DirModel::isLoadingChanged);
    connect(m_lister, &KDirLister::jobError, this, [this](KIO::Job *job) {
        m_lastError = job->errorString();
        Q_EMIT lastErrorChanged();
    });
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    switch (role) {
    case Name:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).name();
    case Url:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).url();
    case IconName:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).iconName();
    case IsDir:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).isDir();
    case IsLink:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).isLink();
    case FileSize:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).size();
    case MimeType:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).mimetype();
    case IsHidden:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).isHidden();
    case IsReadable:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).isReadable();
    case IsWritable:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).isWritable();
    case ModificationTime:
        return qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)).time(KFileItem::ModificationTime);
    case Thumbnail: {
        // return a null QVariant instead of a null QPixmap
        const auto pixmap = m_previews.value(qvariant_cast<KFileItem>(KDirSortFilterProxyModel::data(index, KDirModel::FileItemRole)));
        if (pixmap.isNull()) {
            return {};
        } else {
            return pixmap;
        }
    }
    }

    return {};
}

QHash<int, QByteArray> DirModel::roleNames() const
{
    return {
        {Name, QByteArrayLiteral("name")},
        {Url, QByteArrayLiteral("url")},
        {IconName, QByteArrayLiteral("iconName")},
        {IsDir, QByteArrayLiteral("isDir")},
        {IsLink, QByteArrayLiteral("isLink")},
        {FileSize, QByteArrayLiteral("fileSize")},
        {MimeType, QByteArrayLiteral("mimeType")},
        {IsHidden, QByteArrayLiteral("isHidden")},
        {IsReadable, QByteArrayLiteral("isReadable")},
        {IsWritable, QByteArrayLiteral("isWritable")},
        {ModificationTime, QByteArrayLiteral("modificationTime")},
        {Thumbnail, QByteArrayLiteral("thumbnail")},
    };
}

QUrl DirModel::folder() const
{
    return m_lister->url();
}

void DirModel::setFolder(const QUrl &folder)
{
    if (folder != m_lister->url()) {
        m_dirModel.openUrl(folder);

        Q_EMIT folderChanged();
    }
}

bool DirModel::isLoading() const
{
    return !m_lister->isFinished();
}

bool DirModel::showDotFiles() const
{
    return m_lister->showingDotFiles();
}

void DirModel::setShowDotFiles(bool showDotFiles)
{
    m_lister->setShowingDotFiles(showDotFiles);
    m_lister->emitChanges();
    Q_EMIT showDotFilesChanged();
}

QString DirModel::nameFilter() const
{
    return m_lister->nameFilter();
}

void DirModel::setNameFilter(const QString &nameFilter)
{
    if (nameFilter != m_lister->nameFilter()) {
        m_lister->setNameFilter(nameFilter);
        m_lister->emitChanges();

        Q_EMIT nameFilterChanged();
    }
}

QStringList DirModel::mimeFilters() const
{
    return m_lister->mimeFilters();
}

void DirModel::setMimeFilters(const QStringList &mimeFilters)
{
    if (mimeFilters != m_lister->mimeFilters()) {
        QStringList newMimeFilters = mimeFilters;
        newMimeFilters.append(QStringLiteral("inode/directory"));

        m_lister->setMimeFilter(newMimeFilters);
        m_lister->emitChanges();

        Q_EMIT mimeFiltersChanged();
    }
}

void DirModel::resetMimeFilters()
{
    m_lister->clearMimeFilter();
    m_lister->emitChanges();
    Q_EMIT mimeFiltersChanged();
}

void DirModel::setThumbnailSize(QSize size)
{
    if (m_thumbnailSize == size) {
        return;
    }

    m_thumbnailSize = size;
    Q_EMIT thumbnailSizeChanged();
}

QString DirModel::lastError() const
{
    return m_lastError;
}
