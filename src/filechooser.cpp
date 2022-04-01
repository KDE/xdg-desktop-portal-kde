/*
 * SPDX-FileCopyrightText: 2016-2018 Red Hat Inc
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
 * SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 */

#include "filechooser.h"
#include "utils.h"

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPushButton>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

#include <KFileFilterCombo>
#include <KFileWidget>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>

#include "fuse_interface.h"
#include <mobilefiledialog.h>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdeFileChooser, "xdp-kde-file-chooser")

// Keep in sync with qflatpakfiledialog from flatpak-platform-plugin
Q_DECLARE_METATYPE(FileChooserPortal::Filter)
Q_DECLARE_METATYPE(FileChooserPortal::Filters)
Q_DECLARE_METATYPE(FileChooserPortal::FilterList)
Q_DECLARE_METATYPE(FileChooserPortal::FilterListList)
// used for options - choices
Q_DECLARE_METATYPE(FileChooserPortal::Choice)
Q_DECLARE_METATYPE(FileChooserPortal::Choices)
Q_DECLARE_METATYPE(FileChooserPortal::Option)
Q_DECLARE_METATYPE(FileChooserPortal::OptionList)

QDBusArgument &operator<<(QDBusArgument &arg, const FileChooserPortal::Filter &filter)
{
    arg.beginStructure();
    arg << filter.type << filter.filterString;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, FileChooserPortal::Filter &filter)
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

QDBusArgument &operator<<(QDBusArgument &arg, const FileChooserPortal::FilterList &filterList)
{
    arg.beginStructure();
    arg << filterList.userVisibleName << filterList.filters;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, FileChooserPortal::FilterList &filterList)
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

QDBusArgument &operator<<(QDBusArgument &arg, const FileChooserPortal::Choice &choice)
{
    arg.beginStructure();
    arg << choice.id << choice.value;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, FileChooserPortal::Choice &choice)
{
    QString id;
    QString value;
    arg.beginStructure();
    arg >> id >> value;
    choice.id = id;
    choice.value = value;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const FileChooserPortal::Option &option)
{
    arg.beginStructure();
    arg << option.id << option.label << option.choices << option.initialChoiceId;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, FileChooserPortal::Option &option)
{
    QString id;
    QString label;
    FileChooserPortal::Choices choices;
    QString initialChoiceId;
    arg.beginStructure();
    arg >> id >> label >> choices >> initialChoiceId;
    option.id = id;
    option.label = label;
    option.choices = choices;
    option.initialChoiceId = initialChoiceId;
    arg.endStructure();
    return arg;
}

static bool isKIOFuseAvailable()
{
    static bool available =
        QDBusConnection::sessionBus().interface() && QDBusConnection::sessionBus().interface()->activatableServiceNames().value().contains("org.kde.KIOFuse");
    return available;
}

FileDialog::FileDialog(QDialog *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_fileWidget(new KFileWidget(QUrl(), this))
    , m_configGroup(KSharedConfig::openConfig()->group("FileDialogSize"))
{
    setLayout(new QVBoxLayout);
    layout()->addWidget(m_fileWidget);

    m_buttons = new QDialogButtonBox(this);
    m_buttons->addButton(m_fileWidget->okButton(), QDialogButtonBox::AcceptRole);
    m_buttons->addButton(m_fileWidget->cancelButton(), QDialogButtonBox::RejectRole);
    connect(m_buttons, &QDialogButtonBox::rejected, m_fileWidget, &KFileWidget::slotCancel);
    connect(m_fileWidget->okButton(), &QAbstractButton::clicked, m_fileWidget, &KFileWidget::slotOk);
    connect(m_fileWidget, &KFileWidget::accepted, m_fileWidget, &KFileWidget::accept);
    connect(m_fileWidget, &KFileWidget::accepted, this, &QDialog::accept);
    connect(m_fileWidget->cancelButton(), &QAbstractButton::clicked, this, &QDialog::reject);
    layout()->addWidget(m_buttons);

    // restore window size
    if (m_configGroup.exists()) {
        winId(); // ensure there's a window created
        KWindowConfig::restoreWindowSize(windowHandle(), m_configGroup);
        resize(windowHandle()->size());
    }
}

FileDialog::~FileDialog()
{
    // save window size
    KWindowConfig::saveWindowSize(windowHandle(), m_configGroup);
}

FileChooserPortal::FileChooserPortal(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    qDBusRegisterMetaType<Filter>();
    qDBusRegisterMetaType<Filters>();
    qDBusRegisterMetaType<FilterList>();
    qDBusRegisterMetaType<FilterListList>();
    qDBusRegisterMetaType<Choice>();
    qDBusRegisterMetaType<Choices>();
    qDBusRegisterMetaType<Option>();
    qDBusRegisterMetaType<OptionList>();
}

FileChooserPortal::~FileChooserPortal()
{
}

static QStringList fuseRedirect(QList<QUrl> urls)
{
    qCDebug(XdgDesktopPortalKdeFileChooser) << "mounting urls with fuse" << urls;

    OrgKdeKIOFuseVFSInterface kiofuse_iface(QStringLiteral("org.kde.KIOFuse"), QStringLiteral("/org/kde/KIOFuse"), QDBusConnection::sessionBus());
    struct MountRequest {
        QDBusPendingReply<QString> reply;
        int urlIndex;
        QString basename;
    };
    QVector<MountRequest> requests;
    requests.reserve(urls.count());
    for (int i = 0; i < urls.count(); ++i) {
        QUrl url = urls.at(i);
        if (!url.isLocalFile()) {
            const QString path(url.path());
            const int slashes = path.count(QLatin1Char('/'));
            QString basename;
            if (slashes > 1) {
                url.setPath(path.section(QLatin1Char('/'), 0, slashes - 1));
                basename = path.section(QLatin1Char('/'), slashes, slashes);
            }
            requests.push_back({kiofuse_iface.mountUrl(url.toString()), i, basename});
        }
    }

    for (auto &request : requests) {
        request.reply.waitForFinished();
        if (request.reply.isError()) {
            qWarning() << "FUSE request failed:" << request.reply.error();
            continue;
        }

        urls[request.urlIndex] = QUrl::fromLocalFile(request.reply.value() + QLatin1Char('/') + request.basename);
    };

    qCDebug(XdgDesktopPortalKdeFileChooser) << "mounted urls with fuse, maybe" << urls;

    return QUrl::toStringList(urls);
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

    bool directory = false;
    bool modalDialog = true;
    bool multipleFiles = false;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString selectedMimeTypeFilter;
    // mapping between filter strings and actual filters
    QMap<QString, FilterList> allFilters;

    const QString acceptLabel = ExtractAcceptLabel(options);

    if (options.contains(QStringLiteral("modal"))) {
        modalDialog = options.value(QStringLiteral("modal")).toBool();
    }

    if (options.contains(QStringLiteral("multiple"))) {
        multipleFiles = options.value(QStringLiteral("multiple")).toBool();
    }

    if (options.contains(QStringLiteral("directory"))) {
        directory = options.value(QStringLiteral("directory")).toBool();
    }

    ExtractFilters(options, nameFilters, mimeTypeFilters, allFilters, selectedMimeTypeFilter);

    if (isMobile()) {
        if (!m_mobileFileDialog) {
            qCDebug(XdgDesktopPortalKdeFileChooser) << "Creating file dialog";
            m_mobileFileDialog = new MobileFileDialog(this);
        }

        m_mobileFileDialog->setTitle(title);

        // Always true when we are opening a file
        m_mobileFileDialog->setSelectExisting(true);

        m_mobileFileDialog->setSelectFolder(directory);

        m_mobileFileDialog->setSelectMultiple(multipleFiles);

        // currentName: not implemented

        if (!acceptLabel.isEmpty()) {
            m_mobileFileDialog->setAcceptLabel(acceptLabel);
        }

        if (!nameFilters.isEmpty()) {
            m_mobileFileDialog->setNameFilters(nameFilters);
        }

        if (!mimeTypeFilters.isEmpty()) {
            m_mobileFileDialog->setMimeTypeFilters(mimeTypeFilters);
        }

        uint retCode = m_mobileFileDialog->exec();

        results.insert(QStringLiteral("uris"), fuseRedirect(m_mobileFileDialog->results()));

        return retCode;
    }

    // Use QFileDialog for most directory requests to utilize
    // plasma-integration's KDirSelectDialog
    if (directory && !options.contains(QStringLiteral("choices"))) {
        QFileDialog dirDialog;
        dirDialog.setWindowTitle(title);
        dirDialog.setModal(modalDialog);
        dirDialog.setFileMode(QFileDialog::Directory);
        dirDialog.setOptions(QFileDialog::ShowDirsOnly);
        if (!isKIOFuseAvailable()) {
            dirDialog.setSupportedSchemes(QStringList{QStringLiteral("file")});
        }
        if (!acceptLabel.isEmpty()) {
            dirDialog.setLabelText(QFileDialog::Accept, acceptLabel);
        }

        dirDialog.winId(); // Trigger window creation
        Utils::setParentWindow(&dirDialog, parent_window);

        if (dirDialog.exec() != QDialog::Accepted) {
            return 1;
        }

        const auto urls = dirDialog.selectedUrls();
        if (urls.empty()) {
            return 2;
        }

        results.insert(QStringLiteral("uris"), fuseRedirect(urls));
        results.insert(QStringLiteral("writable"), true);

        return 0;
    }

    // for handling of options - choices
    QScopedPointer<QWidget> optionsWidget;
    // to store IDs for choices along with corresponding comboboxes/checkboxes
    QMap<QString, QCheckBox *> checkboxes;
    QMap<QString, QComboBox *> comboboxes;

    if (options.contains(QStringLiteral("choices"))) {
        OptionList optionList = qdbus_cast<OptionList>(options.value(QStringLiteral("choices")));
        optionsWidget.reset(CreateChoiceControls(optionList, checkboxes, comboboxes));
    }

    QScopedPointer<FileDialog, QScopedPointerDeleteLater> fileDialog(new FileDialog());
    Utils::setParentWindow(fileDialog.data(), parent_window);
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    KFile::Mode mode = directory ? KFile::Mode::Directory : multipleFiles ? KFile::Mode::Files : KFile::Mode::File;
    fileDialog->m_fileWidget->setMode(mode | KFile::Mode::ExistingOnly);
    if (!isKIOFuseAvailable()) {
        fileDialog->m_fileWidget->setSupportedSchemes(QStringList{QStringLiteral("file")});
    }
    fileDialog->m_fileWidget->okButton()->setText(!acceptLabel.isEmpty() ? acceptLabel : i18n("Open"));

    bool bMimeFilters = false;
    if (!mimeTypeFilters.isEmpty()) {
        fileDialog->m_fileWidget->setMimeFilter(mimeTypeFilters, selectedMimeTypeFilter);
        bMimeFilters = true;
    } else if (!nameFilters.isEmpty()) {
        fileDialog->m_fileWidget->setFilter(nameFilters.join(QLatin1Char('\n')));
    }

    if (optionsWidget) {
        fileDialog->m_fileWidget->setCustomWidget({}, optionsWidget.get());
    }

    if (fileDialog->exec() == QDialog::Accepted) {
        const auto urls = fileDialog->m_fileWidget->selectedUrls();
        if (urls.isEmpty()) {
            qCDebug(XdgDesktopPortalKdeFileChooser) << "Failed to open file: no local file selected";
            return 2;
        }

        results.insert(QStringLiteral("uris"), fuseRedirect(urls));
        results.insert(QStringLiteral("writable"), true);

        if (optionsWidget) {
            QVariant choices = EvaluateSelectedChoices(checkboxes, comboboxes);
            results.insert(QStringLiteral("choices"), choices);
        }

        // try to map current filter back to one of the predefined ones
        QString selectedFilter;
        if (bMimeFilters) {
            selectedFilter = fileDialog->m_fileWidget->currentMimeFilter();
        } else {
            selectedFilter = fileDialog->m_fileWidget->filterWidget()->currentText();
        }
        if (allFilters.contains(selectedFilter)) {
            results.insert(QStringLiteral("current_filter"), QVariant::fromValue<FilterList>(allFilters.value(selectedFilter)));
        }

        return 0;
    }

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
    QString currentName;
    QString currentFolder;
    QString currentFile;
    QStringList nameFilters;
    QStringList mimeTypeFilters;
    QString selectedMimeTypeFilter;
    // mapping between filter strings and actual filters
    QMap<QString, FilterList> allFilters;

    if (options.contains(QStringLiteral("modal"))) {
        modalDialog = options.value(QStringLiteral("modal")).toBool();
    }

    const QString acceptLabel = ExtractAcceptLabel(options);

    if (options.contains(QStringLiteral("current_name"))) {
        currentName = options.value(QStringLiteral("current_name")).toString();
    }

    if (options.contains(QStringLiteral("current_folder"))) {
        currentFolder = QFile::decodeName(options.value(QStringLiteral("current_folder")).toByteArray());
    }

    if (options.contains(QStringLiteral("current_file"))) {
        currentFile = QFile::decodeName(options.value(QStringLiteral("current_file")).toByteArray());
    }

    ExtractFilters(options, nameFilters, mimeTypeFilters, allFilters, selectedMimeTypeFilter);

    if (isMobile()) {
        if (!m_mobileFileDialog) {
            qCDebug(XdgDesktopPortalKdeFileChooser) << "Creating file dialog";
            m_mobileFileDialog = new MobileFileDialog(this);
        }

        m_mobileFileDialog->setTitle(title);

        // Always false when we are saving a file
        m_mobileFileDialog->setSelectExisting(false);

        if (!currentFolder.isEmpty()) {
            // Set correct protocol
            m_mobileFileDialog->setFolder(QUrl::fromLocalFile(currentFolder));
        }

        if (!currentFile.isEmpty()) {
            m_mobileFileDialog->setCurrentFile(currentFile);
        }

        // currentName: not implemented

        if (!acceptLabel.isEmpty()) {
            m_mobileFileDialog->setAcceptLabel(acceptLabel);
        }

        if (!nameFilters.isEmpty()) {
            m_mobileFileDialog->setNameFilters(nameFilters);
        }

        if (!mimeTypeFilters.isEmpty()) {
            m_mobileFileDialog->setMimeTypeFilters(mimeTypeFilters);
        }

        uint retCode = m_mobileFileDialog->exec();

        results.insert(QStringLiteral("uris"), fuseRedirect(m_mobileFileDialog->results()));

        return retCode;
    }

    // for handling of options - choices
    QScopedPointer<QWidget> optionsWidget;
    // to store IDs for choices along with corresponding comboboxes/checkboxes
    QMap<QString, QCheckBox *> checkboxes;
    QMap<QString, QComboBox *> comboboxes;

    if (options.contains(QStringLiteral("choices"))) {
        OptionList optionList = qdbus_cast<OptionList>(options.value(QStringLiteral("choices")));
        optionsWidget.reset(CreateChoiceControls(optionList, checkboxes, comboboxes));
    }

    QScopedPointer<FileDialog, QScopedPointerDeleteLater> fileDialog(new FileDialog());
    Utils::setParentWindow(fileDialog.data(), parent_window);
    fileDialog->setWindowTitle(title);
    fileDialog->setModal(modalDialog);
    fileDialog->m_fileWidget->setOperationMode(KFileWidget::Saving);
    fileDialog->m_fileWidget->setConfirmOverwrite(true);

    if (!currentFolder.isEmpty()) {
        fileDialog->m_fileWidget->setUrl(QUrl::fromLocalFile(currentFolder));
    }

    if (!currentFile.isEmpty()) {
        fileDialog->m_fileWidget->setSelectedUrl(QUrl::fromLocalFile(currentFile));
    }

    if (!currentName.isEmpty()) {
        const QUrl url = fileDialog->m_fileWidget->baseUrl();
        fileDialog->m_fileWidget->setSelectedUrl(QUrl::fromLocalFile(QStringLiteral("%1/%2").arg(url.toDisplayString(QUrl::StripTrailingSlash), currentName)));
    }

    if (!acceptLabel.isEmpty()) {
        fileDialog->m_fileWidget->okButton()->setText(acceptLabel);
    }

    bool bMimeFilters = false;
    if (!mimeTypeFilters.isEmpty()) {
        fileDialog->m_fileWidget->setMimeFilter(mimeTypeFilters, selectedMimeTypeFilter);
        bMimeFilters = true;
    } else if (!nameFilters.isEmpty()) {
        fileDialog->m_fileWidget->setFilter(nameFilters.join(QLatin1Char('\n')));
    }

    if (optionsWidget) {
        fileDialog->m_fileWidget->setCustomWidget(optionsWidget.get());
    }

    if (fileDialog->exec() == QDialog::Accepted) {
        const auto urls = fileDialog->m_fileWidget->selectedUrls();
        results.insert(QStringLiteral("uris"), fuseRedirect(urls));

        if (optionsWidget) {
            QVariant choices = EvaluateSelectedChoices(checkboxes, comboboxes);
            results.insert(QStringLiteral("choices"), choices);
        }

        // try to map current filter back to one of the predefined ones
        QString selectedFilter;
        if (bMimeFilters) {
            selectedFilter = fileDialog->m_fileWidget->currentMimeFilter();
        } else {
            selectedFilter = fileDialog->m_fileWidget->filterWidget()->currentText();
        }
        if (allFilters.contains(selectedFilter)) {
            results.insert(QStringLiteral("current_filter"), QVariant::fromValue<FilterList>(allFilters.value(selectedFilter)));
        }

        return 0;
    }

    return 1;
}

QWidget *FileChooserPortal::CreateChoiceControls(const FileChooserPortal::OptionList &optionList,
                                                 QMap<QString, QCheckBox *> &checkboxes,
                                                 QMap<QString, QComboBox *> &comboboxes)
{
    if (optionList.empty()) {
        return nullptr;
    }

    QWidget *optionsWidget = new QWidget;
    QGridLayout *layout = new QGridLayout(optionsWidget);
    // set stretch for (unused) column 2 so controls only take the space they actually need
    layout->setColumnStretch(2, 1);
    optionsWidget->setLayout(layout);

    for (const Option &option : optionList) {
        const int nextRow = layout->rowCount();
        // empty list of choices -> boolean choice according to the spec
        if (option.choices.empty()) {
            QCheckBox *checkbox = new QCheckBox(option.label, optionsWidget);
            checkbox->setChecked(option.initialChoiceId == QStringLiteral("true"));
            layout->addWidget(checkbox, nextRow, 1);
            checkboxes.insert(option.id, checkbox);
        } else {
            QComboBox *combobox = new QComboBox(optionsWidget);
            for (const Choice &choice : option.choices) {
                combobox->addItem(choice.value, choice.id);
                // select this entry if initialChoiceId matches
                if (choice.id == option.initialChoiceId) {
                    combobox->setCurrentIndex(combobox->count() - 1);
                }
            }
            QString labelText = option.label;
            if (!labelText.endsWith(QChar::fromLatin1(':'))) {
                labelText += QChar::fromLatin1(':');
            }
            QLabel *label = new QLabel(labelText, optionsWidget);
            label->setBuddy(combobox);
            layout->addWidget(label, nextRow, 0, Qt::AlignRight);
            layout->addWidget(combobox, nextRow, 1);
            comboboxes.insert(option.id, combobox);
        }
    }

    return optionsWidget;
}

QVariant FileChooserPortal::EvaluateSelectedChoices(const QMap<QString, QCheckBox *> &checkboxes, const QMap<QString, QComboBox *> &comboboxes)
{
    Choices selectedChoices;
    const auto checkboxKeys = checkboxes.keys();
    for (const QString &id : checkboxKeys) {
        Choice choice;
        choice.id = id;
        choice.value = checkboxes.value(id)->isChecked() ? QStringLiteral("true") : QStringLiteral("false");
        selectedChoices << choice;
    }
    const auto comboboxKeys = comboboxes.keys();
    for (const QString &id : comboboxKeys) {
        Choice choice;
        choice.id = id;
        choice.value = comboboxes.value(id)->currentData().toString();
        selectedChoices << choice;
    }

    return QVariant::fromValue<Choices>(selectedChoices);
}

QString FileChooserPortal::ExtractAcceptLabel(const QVariantMap &options)
{
    QString acceptLabel;
    if (options.contains(QStringLiteral("accept_label"))) {
        acceptLabel = options.value(QStringLiteral("accept_label")).toString();
        // 'accept_label' allows mnemonic underlines, but Qt uses '&' character, so replace/escape accordingly
        // to keep literal '&'s and transform mnemonic underlines to the Qt equivalent using '&' for mnemonic
        acceptLabel.replace(QChar::fromLatin1('&'), QStringLiteral("&&"));
        const int mnemonic_pos = acceptLabel.indexOf(QChar::fromLatin1('_'));
        if (mnemonic_pos != -1) {
            acceptLabel.replace(mnemonic_pos, 1, QChar::fromLatin1('&'));
        }
    }
    return acceptLabel;
}

void FileChooserPortal::ExtractFilters(const QVariantMap &options,
                                       QStringList &nameFilters,
                                       QStringList &mimeTypeFilters,
                                       QMap<QString, FilterList> &allFilters,
                                       QString &selectedMimeTypeFilter)
{
    if (options.contains(QStringLiteral("filters"))) {
        const FilterListList filterListList = qdbus_cast<FilterListList>(options.value(QStringLiteral("filters")));
        for (const FilterList &filterList : filterListList) {
            QStringList filterStrings;
            for (const Filter &filterStruct : filterList.filters) {
                if (filterStruct.type == 0) {
                    filterStrings << filterStruct.filterString;
                } else {
                    mimeTypeFilters << filterStruct.filterString;
                    allFilters[filterStruct.filterString] = filterList;
                }
            }

            if (!filterStrings.isEmpty()) {
                QString userVisibleName = filterList.userVisibleName;
                if (!isMobile()) {
                    userVisibleName.replace(QLatin1Char('/'), QStringLiteral("\\/"));
                }
                const QString filterString = filterStrings.join(QLatin1Char(' '));
                const QString nameFilter = QStringLiteral("%1|%2").arg(filterString, userVisibleName);
                nameFilters << nameFilter;
                allFilters[filterList.userVisibleName] = filterList;
            }
        }
    }

    if (options.contains(QStringLiteral("current_filter"))) {
        FilterList filterList = qdbus_cast<FilterList>(options.value(QStringLiteral("current_filter")));
        if (filterList.filters.size() == 1) {
            Filter filterStruct = filterList.filters.at(0);
            if (filterStruct.type == 0) {
                // make the relevant entry the first one in the list of filters,
                // since that is the one that gets preselected by KFileWidget::setFilter
                QString userVisibleName = filterList.userVisibleName;
                if (!isMobile()) {
                    userVisibleName.replace(QLatin1Char('/'), QStringLiteral("\\/"));
                }
                QString nameFilter = QStringLiteral("%1|%2").arg(filterStruct.filterString, userVisibleName);
                nameFilters.removeAll(nameFilter);
                nameFilters.push_front(nameFilter);
            } else {
                selectedMimeTypeFilter = filterStruct.filterString;
            }
        } else {
            qCDebug(XdgDesktopPortalKdeFileChooser) << "Ignoring 'current_filter' parameter with 0 or multiple filters specified.";
        }
    }
}

bool FileChooserPortal::isMobile()
{
    QByteArray mobile = qgetenv("QT_QUICK_CONTROLS_MOBILE");
    return mobile == "true" || mobile == "1";
}
