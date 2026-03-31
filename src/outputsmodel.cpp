/*
 * SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "outputsmodel.h"
#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QWaylandClientExtensionTemplate>

#include <algorithm>
#include <ranges>

#include "debug.h"
#include "qwayland-kde-output-order-v1.h"

using namespace Qt::StringLiterals;

namespace
{
template<typename Output, typename Input>
Output narrow(Input i)
{
    Output o = i;
    if (i != Input(o)) {
        std::abort();
    }
    if (const auto sameSignedness = (std::is_signed_v<Input> && std::is_signed_v<Output>); !sameSignedness && ((i < Input{}) != (o < Output{}))) {
        std::abort();
    }
    return o;
}
} // namespace

class OutputOrder : public QWaylandClientExtensionTemplate<OutputOrder, &QtWayland::kde_output_order_v1::destroy>, public QtWayland::kde_output_order_v1
{
    Q_OBJECT
public:
    using QWaylandClientExtensionTemplate::QWaylandClientExtensionTemplate;

    bool m_streaming = false;
    std::optional<QList<QString>> m_outputs;

Q_SIGNALS:
    void outputsChanged();

protected:
    void kde_output_order_v1_output(const QString &output_name) override
    {
        if (!m_streaming) {
            m_outputs = QList<QString>(); // (re)set
            m_streaming = true;
        }
        m_outputs->append(output_name);
    }

    void kde_output_order_v1_done() override
    {
        Q_ASSERT(m_streaming);
        m_streaming = false;
        Q_EMIT outputsChanged();
    }
};

OutputsModel::OutputsModel(Options o, QObject *parent)
    : QAbstractListModel(parent)
    , m_outputOrder(std::make_unique<OutputOrder>(1))
{
    // Be mindful that all output changes trigger the slot. Otherwise we may end up with unsorted outputs!
    // Specifically should we ever need to listen to QGuiApplication::screenAdded.
    connect(m_outputOrder.get(), &OutputOrder::outputsChanged, this, &OutputsModel::onOutputsChanged);

    if (o & VirtualIncluded) {
        m_outputs << Output{Output::Virtual, nullptr, i18n("Share virtual screen"), QStringLiteral("Virtual"), {}, nullptr};
    }

    if (o & RegionIncluded) {
        m_outputs << Output{Output::Region, nullptr, i18n("Share region"), u"Region"_s, {}, nullptr};
    }

    if (o & OutputsExcluded) {
        return;
    }

    const auto screens = qGuiApp->screens();

    // Only show the full workspace if there's several outputs
    if (screens.count() > 1 && (o & WorkspaceIncluded)) {
        m_outputs.prepend(Output{Output::Workspace, nullptr, i18n("Share full Workspace"), u"Workspace"_s, {}, nullptr});
    }

    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
        auto it = std::ranges::find(m_outputs, screen, &Output::screen);
        if (it != m_outputs.end()) {
            auto index = std::distance(m_outputs.begin(), it);
            beginRemoveRows(QModelIndex(), index, index);
            m_outputs.erase(it);
            endRemoveRows();
        }
    });

    for (const auto &screen : screens) {
        Output::OutputType type = Output::Unknown;

        static const auto embedded = {
            QLatin1String("LVDS"),
            QLatin1String("IDP"),
            QLatin1String("EDP"),
            QLatin1String("LCD"),
        };

        if (std::ranges::any_of(embedded, [screen](const QString &prefix) {
                return screen->name().startsWith(prefix, Qt::CaseInsensitive);
            })) {
            type = Output::OutputType::Laptop;
            break;
        }

        if (screen->name().contains(QLatin1String("TV"))) {
            type = Output::OutputType::Television;
        } else {
            type = Output::OutputType::Monitor;
        }

        QString displayText;
        if (type == Output::OutputType::Laptop) {
            displayText = i18n("Laptop screen");
        } else {
            QStringList parts;
            if (!screen->manufacturer().isEmpty()) {
                parts.append(screen->manufacturer());

                if (!screen->model().isEmpty()) {
                    QString part = screen->model();
                    if (!screen->serialNumber().isEmpty()) {
                        part += QLatin1Char('/') + screen->serialNumber();
                    }
                    parts.append(part);
                } else if (!screen->serialNumber().isEmpty()) {
                    parts.append(screen->serialNumber());
                }

                parts.append(QLatin1Char('(') + screen->name() + QLatin1Char(')'));
            } else {
                parts.append(screen->name());
            }

            displayText = parts.join(QLatin1Char(' '));
        }

        const QPoint pos = screen->geometry().topLeft();
        const QString uniqueId = QStringLiteral("%1x%2").arg(pos.x()).arg(pos.y());

        // set up an async wallpaper grab
        QString dirPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + u"/xdg-desktop-portal-kde/screen-grabs";
        QDir().mkpath(dirPath);
        QString path = dirPath + u"/%1.XXXXXX.webp"_s.arg(uniqueId);
        auto temporaryFile = std::make_shared<QTemporaryFile>(path);
        if (!temporaryFile->open()) {
            qCWarning(XdgDesktopPortalKde) << "Failed to create temporary file for screen grab of output" << uniqueId << ":" << temporaryFile->errorString();
            continue;
        }

        auto msg = QDBusMessage::createMethodCall(u"org.kde.plasmashell"_s, u"/PlasmaShell"_s, u"org.kde.PlasmaShell"_s, u"grabContainmentImage"_s);
        msg << screen->name() << /* width= */ 0 << /* height= */ 0 << temporaryFile->fileName();
        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
        connect(
            watcher,
            &QDBusPendingCallWatcher::finished,
            this,
            [this, uniqueId, watcher, temporaryFile]() {
                watcher->deleteLater();
                const QDBusReply<bool> reply = watcher->reply();
                if (!reply.isValid()) {
                    qWarning() << "Failed to get desktop containment image for output" << uniqueId << ":" << reply.error();
                    return;
                }

                // freebsd doesn't have std::views::enumerate. It'd be vastly more convenient :( - 2026-03-24
                for (const auto &[outputIndex, output] : std::views::zip(std::views::iota(0), m_outputs)) {
                    if (output.uniqueId() == uniqueId) {
                        output.setImage(QUrl::fromLocalFile(temporaryFile->fileName()));
                        const auto idx = index(narrow<int>(outputIndex), 0);
                        Q_EMIT dataChanged(idx, idx, {ImageUrlRole});
                        break;
                    }
                }
            },
            Qt::SingleShotConnection);

        m_outputs << Output(type, screen, displayText, uniqueId, screen->name(), temporaryFile);
    }

    // Partition so that real outputs come first. This is the order in which we want to visualize them in the UI.
    std::ranges::stable_partition(m_outputs, [](const Output &o) {
        return !o.isSynthetic();
    });
}

OutputsModel::~OutputsModel() = default;

int OutputsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_outputs.count();
}

QHash<int, QByteArray> OutputsModel::roleNames() const
{
    return QHash<int, QByteArray>{
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {Qt::CheckStateRole, "checked"},
        {ScreenRole, "screen"},
        {NameRole, "name"},
        {IsSyntheticRole, "isSynthetic"},
        {DescriptionRole, "description"},
        {GeometryRole, "geometry"},
        {ImageUrlRole, "imageUrl"},
    };
}

QVariant OutputsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto &output = m_outputs[index.row()];
    switch (role) {
    case ScreenRole:
        return QVariant::fromValue(output.screen());
    case NameRole:
        return output.name();
    case IsSyntheticRole:
        return output.isSynthetic();
    case DescriptionRole:
        return output.description();
    case GeometryRole:
        return output.screen() ? output.screen()->geometry() : QRect();
    case ImageUrlRole:
        return output.imageUrl();
    case Qt::DecorationRole:
        return QIcon::fromTheme(output.iconName());
    case Qt::DisplayRole:
        return output.display();
    case Qt::CheckStateRole:
        return m_selectedRows.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
    }
    return {};
}

bool OutputsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
        return false;
    }

    if (index.data(Qt::CheckStateRole) == value) {
        return true;
    }

    if (value == Qt::Checked) {
        m_selectedRows.insert(index.row());
    } else {
        m_selectedRows.remove(index.row());
    }
    Q_EMIT dataChanged(index, index, {role});
    if (m_selectedRows.count() <= 1) {
        Q_EMIT hasSelectionChanged();
    }
    return true;
}

const Output &OutputsModel::outputAt(int row) const
{
    return m_outputs[row];
}

void OutputsModel::clearSelection()
{
    if (m_selectedRows.isEmpty())
        return;

    auto selected = m_selectedRows;
    m_selectedRows.clear();
    for (int i = 0, c = rowCount({}); i < c; ++i) {
        if (selected.contains(i)) {
            const auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {Qt::CheckStateRole});
        }
    }
    Q_EMIT hasSelectionChanged();
}

void OutputsModel::onOutputsChanged()
{
    const auto order = m_outputOrder->m_outputs;
    if (!order) { // We don't have an order yet, we'll run again when there is one.
        return;
    }

    // Order doesn't change nearly enough to justify a more surgical approach, reset the entire model.
    beginResetModel();
    std::ranges::stable_sort(m_outputs, [&order](const Output &a, const Output &b) {
        const auto indexA = std::ranges::find(*order, a.name());
        const auto indexB = std::ranges::find(*order, b.name());
        if (indexA == order->end() || indexB == order->end()) {
            return false;
        }
        return std::distance(order->begin(), indexA) < std::distance(order->begin(), indexB);
    });
    endResetModel();
}

QList<Output> OutputsModel::selectedOutputs() const
{
    QList<Output> ret;
    ret.reserve(m_selectedRows.count());
    for (auto x : std::as_const(m_selectedRows)) {
        ret << m_outputs[x];
    }
    return ret;
}

QString OutputsModel::virtualScreenIdForApp(const QString &appId)
{
    const QString baseId = QStringLiteral("virtual-xdp-kde-") + appId;
    QString id = baseId;
    int i = 1;
    const auto screens = qApp->screens();
    while (std::ranges::find(screens, id, &QScreen::name) != std::ranges::end(screens)) {
        id = baseId + u'-' + QString::number(i);
        ++i;
    }
    return id;
}

QString Output::iconName() const
{
    switch (m_outputType) {
    case Laptop:
        return QStringLiteral("computer-laptop-symbolic");
    case Television:
        return QStringLiteral("video-television-symbolic");
    case Region:
        return QStringLiteral("transform-crop-symbolic");
    case Virtual:
        return QStringLiteral("window-duplicate-symbolic");
    case Workspace:
        return QStringLiteral("preferences-desktop-display-randr-symbolic");
    case Monitor:
        return QStringLiteral("monitor-symbolic");
    case Unknown:
        break;
    }
    return QStringLiteral("video-display-symbolic");
}

QString Output::description() const
{
    switch (m_outputType) {
    case Workspace:
        return i18nc("@info", "Share the entire workspace across all screens");
    case Virtual:
        return i18nc("@info", "Creates a screen inside a window, then share");
    case Region:
        return i18nc("@info", "Crops a specific area of your screens");
    case Laptop:
    case Monitor:
    case Television:
    case Unknown:
        // Unused
        break;
    }
    return {};
}

bool Output::isSynthetic() const
{
    switch (outputType()) {
    case Output::Workspace:
    case Output::Region:
    case Output::Virtual:
        return true;
    case Output::Laptop:
    case Output::Monitor:
    case Output::Television:
    case Output::Unknown:
        break;
    }
    return false;
}

#include "moc_outputsmodel.cpp"
#include "outputsmodel.moc"
