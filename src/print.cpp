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

#include "print.h"

#include <QLoggingCategory>
#include <QPrinter>
#include <QPrintDialog>
// #include <QtPrintSupport/private/qprint_p.h>
#include <QtPrintSupport/private/qcups_p.h>
#include <QUrl>

Q_LOGGING_CATEGORY(XdgDesktopPortalKdePrint, "xdg-desktop-portal-kde-print")

// INFO: code below is copied from Qt as there is no public API for converting key to PageSizeId

// TODO seems to by Windows only stuff
// Standard sources data
// struct StandardPaperSource {
//     QPrinter::PaperSource sourceNum;
//     const char *source;
// }
//
// static const StandardPaperSource qt_paperSources[] = {
//     {QPrinter::Auto, "Auto"},
//     {QPrinter::Cassette, "Cassette"},
//     {QPrinter::Envelope, "Envelope"},
//     {QPrinter::EnvelopeManual, "EnvelopeManual"},
//     {QPrinter::FormSource, "FormSource"},
//     {QPrinter::LargeCapacity, "LargeCapacity"},
//     {QPrinter::LargeFormat, "AnyLargeFormat"},
//     {QPrinter::Lower, "Lower"},
//     {QPrinter::Middle, "Middle"},
//     {QPrinter::Manual, "Manual"},
//     {QPrinter::Manual, "ManualFeed"},
//     {QPrinter::OnlyOne, "OnlyOne"}, // = QPrint::Upper
//     {QPrinter::Tractor, "Tractor"},
//     {QPrinter::SmallFormat, "AnySmallFormat"},
//     {QPrinter::Upper, "Upper"},
// };

// Standard sizes data
struct StandardPageSize {
    QPageSize::PageSizeId id;
    const char *mediaOption;  // PPD standard mediaOption ID
};

// Standard page sizes taken from the Postscript PPD Standard v4.3
// See http://partners.adobe.com/public/developer/en/ps/5003.PPD_Spec_v4.3.pdf
// Excludes all Transverse and Rotated sizes
// NB! This table needs to be in sync with QPageSize::PageSizeId
static const StandardPageSize qt_pageSizes[] = {

    // Existing Qt sizes including ISO, US, ANSI and other standards
    {QPageSize::A4, "A4"},
    {QPageSize::B5, "ISOB5"},
    {QPageSize::Letter, "Letter"},
    {QPageSize::Legal, "Legal"},
    {QPageSize::Executive, "Executive.7.5x10in"}, // Qt size differs from Postscript / Windows
    {QPageSize::A0, "A0"},
    {QPageSize::A1, "A1"},
    {QPageSize::A2, "A2"},
    {QPageSize::A3, "A3"},
    {QPageSize::A5, "A5"},
    {QPageSize::A6, "A6"},
    {QPageSize::A7, "A7"},
    {QPageSize::A8, "A8"},
    {QPageSize::A9, "A9"},
    {QPageSize::B0, "ISOB0"},
    {QPageSize::B1, "ISOB1"},
    {QPageSize::B10, "ISOB10"},
    {QPageSize::B2, "ISOB2"},
    {QPageSize::B3, "ISOB3"},
    {QPageSize::B4, "ISOB4"},
    {QPageSize::B6, "ISOB6"},
    {QPageSize::B7, "ISOB7"},
    {QPageSize::B8, "ISOB8"},
    {QPageSize::B9, "ISOB9"},
    {QPageSize::C5E, "EnvC5"},
    {QPageSize::Comm10E, "Env10"},
    {QPageSize::DLE, "EnvDL"},
    {QPageSize::Folio, "Folio"},
    {QPageSize::Ledger, "Ledger"},
    {QPageSize::Tabloid, "Tabloid"},
    {QPageSize::Custom, "Custom"}, // Special case to keep in sync with QPageSize::PageSizeId

    // ISO Standard Sizes
    {QPageSize::A10, "A10"},
    {QPageSize::A3Extra, "A3Extra"},
    {QPageSize::A4Extra, "A4Extra"},
    {QPageSize::A4Plus, "A4Plus"},
    {QPageSize::A4Small, "A4Small"},
    {QPageSize::A5Extra, "A5Extra"},
    {QPageSize::B5Extra, "ISOB5Extra"},

    // JIS Standard Sizes
    {QPageSize::JisB0, "B0"},
    {QPageSize::JisB1, "B1"},
    {QPageSize::JisB2, "B2"},
    {QPageSize::JisB3, "B3"},
    {QPageSize::JisB4, "B4"},
    {QPageSize::JisB5, "B5"},
    {QPageSize::JisB6, "B6"},
    {QPageSize::JisB7, "B7"},
    {QPageSize::JisB8, "B8"},
    {QPageSize::JisB9, "B9"},
    {QPageSize::JisB10, "B10"},

    // ANSI / US Standard sizes
    {QPageSize::AnsiC, "AnsiC"},
    {QPageSize::AnsiD, "AnsiD"},
    {QPageSize::AnsiE, "AnsiE"},
    {QPageSize::LegalExtra, "LegalExtra"},
    {QPageSize::LetterExtra, "LetterExtra"},
    {QPageSize::LetterPlus, "LetterPlus"},
    {QPageSize::LetterSmall, "LetterSmall"},
    {QPageSize::TabloidExtra, "TabloidExtra"},

    // Architectural sizes
    {QPageSize::ArchA, "ARCHA"},
    {QPageSize::ArchB, "ARCHB"},
    {QPageSize::ArchC, "ARCHC"},
    {QPageSize::ArchD, "ARCHD"},
    {QPageSize::ArchE, "ARCHE"},

    // Inch-based Sizes
    {QPageSize::Imperial7x9, "7x9"},
    {QPageSize::Imperial8x10, "8x10"},
    {QPageSize::Imperial9x11, "9x11"},
    {QPageSize::Imperial9x12, "9x12"},
    {QPageSize::Imperial10x11, "10x11"},
    {QPageSize::Imperial10x13, "10x13"},
    {QPageSize::Imperial10x14, "10x14"},
    {QPageSize::Imperial12x11, "12x11"},
    {QPageSize::Imperial15x11, "15x11"},

    // Other Page Sizes
    {QPageSize::ExecutiveStandard, "Executive"},   // Qt size differs from Postscript / Windows
    {QPageSize::Note, "Note"},
    {QPageSize::Quarto, "Quarto"},
    {QPageSize::Statement, "Statement"},
    {QPageSize::SuperA, "SuperA"},
    {QPageSize::SuperB, "SuperB"},
    {QPageSize::Postcard, "Postcard"},
    {QPageSize::DoublePostcard, "DoublePostcard"},
    {QPageSize::Prc16K, "PRC16K"},
    {QPageSize::Prc32K, "PRC32K"},
    {QPageSize::Prc32KBig, "PRC32KBig"},

    // Fan Fold Sizes
    {QPageSize::FanFoldUS, "FanFoldUS"},
    {QPageSize::FanFoldGerman, "FanFoldGerman"},
    {QPageSize::FanFoldGermanLegal, "FanFoldGermanLegal"},

    // ISO Envelopes
    {QPageSize::EnvelopeB4, "EnvISOB4"},
    {QPageSize::EnvelopeB5, "EnvISOB5"},
    {QPageSize::EnvelopeB6, "EnvISOB6"},
    {QPageSize::EnvelopeC0, "EnvC0"},
    {QPageSize::EnvelopeC1, "EnvC1"},
    {QPageSize::EnvelopeC2, "EnvC2"},
    {QPageSize::EnvelopeC3, "EnvC3"},
    {QPageSize::EnvelopeC4, "EnvC4"},
    {QPageSize::EnvelopeC6, "EnvC6"},
    {QPageSize::EnvelopeC65, "EnvC65"},
    {QPageSize::EnvelopeC7, "EnvC7"},

    // US Envelopes
    {QPageSize::Envelope9, "Env9"},
    {QPageSize::Envelope11, "Env11"},
    {QPageSize::Envelope12, "Env12"},
    {QPageSize::Envelope14, "Env14"},
    {QPageSize::EnvelopeMonarch, "EnvMonarch"},
    {QPageSize::EnvelopePersonal, "EnvPersonal"},

    // Other Envelopes
    {QPageSize::EnvelopeChou3, "EnvChou3"},
    {QPageSize::EnvelopeChou4, "EnvChou4"},
    {QPageSize::EnvelopeInvite, "EnvInvite"},
    {QPageSize::EnvelopeItalian, "EnvItalian"},
    {QPageSize::EnvelopeKaku2, "EnvKaku2"},
    {QPageSize::EnvelopeKaku3, "EnvKaku3"},
    {QPageSize::EnvelopePrc1, "EnvPRC1"},
    {QPageSize::EnvelopePrc2, "EnvPRC2"},
    {QPageSize::EnvelopePrc3, "EnvPRC3"},
    {QPageSize::EnvelopePrc4, "EnvPRC4"},
    {QPageSize::EnvelopePrc5, "EnvPRC5"},
    {QPageSize::EnvelopePrc6, "EnvPRC6"},
    {QPageSize::EnvelopePrc7, "EnvPRC7"},
    {QPageSize::EnvelopePrc8, "EnvPRC8"},
    {QPageSize::EnvelopePrc9, "EnvPRC9"},
    {QPageSize::EnvelopePrc10, "EnvPRC10"},
    {QPageSize::EnvelopeYou4, "EnvYou4"}
};

// Return key name for PageSize
static QString qt_keyForPageSizeId(QPageSize::PageSizeId id)
{
    return QString::fromLatin1(qt_pageSizes[id].mediaOption);
}

// Return id name for PPD Key
static QPageSize::PageSizeId qt_idForPpdKey(const QString &ppdKey)
{
    if (ppdKey.isEmpty()) {
        return QPageSize::Custom;
    }

    for (int i = 0; i <= int(QPageSize::LastPageSize); ++i) {
        if (QLatin1String(qt_pageSizes[i].mediaOption) == ppdKey) {
            return qt_pageSizes[i].id;
        }
    }
    return QPageSize::Custom;
}

Print::Print(QObject *parent)
    : QObject(parent)
{
}

Print::~Print()
{
}

uint Print::print(const QDBusObjectPath &handle,
                  const QString &app_id,
                  const QString &parent_window,
                  const QString &title,
                  const QDBusUnixFileDescriptor &fd,
                  const QVariantMap &options,
                  QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdePrint) << "Print called with parameters:";
    qCDebug(XdgDesktopPortalKdePrint) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdePrint) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdePrint) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdePrint) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdePrint) << "    fd: " << fd.fileDescriptor();
    qCDebug(XdgDesktopPortalKdePrint) << "    options: " << options;

    return 0;
}

uint Print::preparePrint(const QDBusObjectPath &handle,
                         const QString &app_id,
                         const QString &parent_window,
                         const QString &title,
                         const QVariantMap &settings,
                         const QVariantMap &page_setup,
                         const QVariantMap &options,
                         QVariantMap &results)
{
    qCDebug(XdgDesktopPortalKdePrint) << "PreparePrint called with parameters:";
    qCDebug(XdgDesktopPortalKdePrint) << "    handle: " << handle.path();
    qCDebug(XdgDesktopPortalKdePrint) << "    app_id: " << app_id;
    qCDebug(XdgDesktopPortalKdePrint) << "    parent_window: " << parent_window;
    qCDebug(XdgDesktopPortalKdePrint) << "    title: " << title;
    qCDebug(XdgDesktopPortalKdePrint) << "    settings: " << settings;
    qCDebug(XdgDesktopPortalKdePrint) << "    page_setup: " << page_setup;
    qCDebug(XdgDesktopPortalKdePrint) << "    options: " << options;

    // Create new one
    QPrinter *printer = new QPrinter();

    // First we have to load pre-configured options

    // Process settings (used by printer)
    if (settings.contains(QLatin1String("orientation"))) {
        // TODO: what is the difference between this and the one in page setup
    }
    if (settings.contains(QLatin1String("paper-format"))) {
        // TODO: what is the difference between this and the one in page setup
    }
    if (settings.contains(QLatin1String("paper-width"))) {
        // TODO: what is the difference between this and the one in page setup
    }
    if (settings.contains(QLatin1String("paper-height"))) {
        // TODO: what is the difference between this and the one in page setup
    }
    if (settings.contains(QLatin1String("n-copies"))) {
        printer->setCopyCount(settings.value(QLatin1String("n-copies")).toString().toInt());
    }
    if (settings.contains(QLatin1String("default-source"))) {
        // TODO seems to be windows only stuff
    }
    if (settings.contains(QLatin1String("quality"))) {
        // TODO doesn't seem to be used by Qt
    }
    if (settings.contains(QLatin1String("resolution"))) {
        printer->setResolution(settings.value(QLatin1String("resolution")).toString().toInt());
    }
    if (settings.contains(QLatin1String("use-color"))) {
        printer->setColorMode(settings.value(QLatin1String("use-color")).toString() == QLatin1String("yes") ? QPrinter::Color : QPrinter::GrayScale);
    }
    if (settings.contains(QLatin1String("duplex"))) {
        const QString duplex = settings.value(QLatin1String("duplex")).toString();
        if (duplex == QLatin1String("simplex")) {
            printer->setDuplex(QPrinter::DuplexNone);
        } else if (duplex == QLatin1String("horizontal")) {
            printer->setDuplex(QPrinter::DuplexShortSide);
        } else if (duplex == QLatin1String("vertical")) {
            printer->setDuplex(QPrinter::DuplexLongSide);
        }
    }
    if (settings.contains(QLatin1String("collate"))) {
        printer->setCollateCopies(settings.value(QLatin1String("collate")).toString() == QLatin1String("yes"));
    }
    if (settings.contains(QLatin1String("reverse"))) {
        printer->setPageOrder(settings.value(QLatin1String("reverse")).toString() == QLatin1String("yes") ? QPrinter::LastPageFirst : QPrinter::FirstPageFirst);
    }

    if (settings.contains(QLatin1String("media-type"))) {
        // TODO doesn't seem to be used by Qt
    }
    if (settings.contains(QLatin1String("dither"))) {
        // TODO doesn't seem to be used by Qt
    }
    if (settings.contains(QLatin1String("scale"))) {
        // TODO doesn't seem to be used by Qt
    }
    if (settings.contains(QLatin1String("print-pages"))) {
        const QString printPages = settings.value(QLatin1String("print-pages")).toString();
        if (printPages == QLatin1String("all")) {
            printer->setPrintRange(QPrinter::AllPages);
        } else if (printPages == QLatin1String("selection")) {
            printer->setPrintRange(QPrinter::Selection);
        } else if (printPages == QLatin1String("current")) {
            printer->setPrintRange(QPrinter::CurrentPage);
        } else if (printPages == QLatin1String("ranges")) {
            printer->setPrintRange(QPrinter::PageRange);
        }
    }
    if (settings.contains(QLatin1String("page-ranges"))) {
        const QString range = settings.value(QLatin1String("page-ranges")).toString();
        // Gnome supports format like 1-5,7,9,11-15, however Qt support only e.g.1-15
        // so get the first and the last value
        const QStringList ranges = range.split(QLatin1Char(','));
        if (ranges.count()) {
            QStringList firstRangeValues = ranges.first().split(QLatin1Char('-'));
            QStringList lastRangeValues = ranges.last().split(QLatin1Char('-'));
            printer->setFromTo(firstRangeValues.first().toInt(), lastRangeValues.last().toInt());
        }
    }
    if (settings.contains(QLatin1String("page-set"))) {
        // WARNING Qt internal private API, anyway the print dialog doesn't seem to
        // read these propertis, but I'll leave it here in case this changes in future
        const QString pageSet = settings.value(QLatin1String("page-set")).toString();
        if (pageSet == QLatin1String("all")) {
            QCUPSSupport::setPageSet(printer, QCUPSSupport::AllPages);
        } else if (pageSet == QLatin1String("even")) {
            QCUPSSupport::setPageSet(printer, QCUPSSupport::EvenPages);
        } else if (pageSet == QLatin1String("odd")) {
            QCUPSSupport::setPageSet(printer, QCUPSSupport::OddPages);
        }
    }
    QCUPSSupport::setPageSet(printer, QCUPSSupport::EvenPages);
    if (settings.contains(QLatin1String("finishings"))) {
        // TODO doesn't seem to be used by Qt
    }

    QCUPSSupport::PagesPerSheet pagesPerSheet = QCUPSSupport::OnePagePerSheet;
    QCUPSSupport::PagesPerSheetLayout pagesPerSheetLayout = QCUPSSupport::LeftToRightTopToBottom;
    if (settings.contains(QLatin1String("number-up"))) {
        // WARNING Qt internal private API, anyway the print dialog doesn't seem to
        // read these propertis, but I'll leave it here in case this changes in future
        const QString numberUp = settings.value(QLatin1String("number-up")).toString();
        if (numberUp == QLatin1String("1")) {
            pagesPerSheet = QCUPSSupport::OnePagePerSheet;
        } else if (numberUp == QLatin1String("2")) {
            pagesPerSheet = QCUPSSupport::TwoPagesPerSheet;
        } else if (numberUp == QLatin1String("4")) {
            pagesPerSheet = QCUPSSupport::FourPagesPerSheet;
        } else if (numberUp == QLatin1String("6")) {
            pagesPerSheet = QCUPSSupport::SixPagesPerSheet;
        } else if (numberUp == QLatin1String("9")) {
            pagesPerSheet = QCUPSSupport::NinePagesPerSheet;
        } else if (numberUp == QLatin1String("16")) {
            pagesPerSheet = QCUPSSupport::SixteenPagesPerSheet;
        }
    }
    if (settings.contains(QLatin1String("number-up-layout"))) {
        // WARNING Qt internal private API, anyway the print dialog doesn't seem to
        // read these propertis, but I'll leave it here in case this changes in future
        const QString layout = settings.value(QLatin1String("number-up-layout")).toString();
        if (layout == QLatin1String("lrtb")) {
            pagesPerSheetLayout = QCUPSSupport::LeftToRightTopToBottom;
        } else if (layout == QLatin1String("lrbt")) {
            pagesPerSheetLayout = QCUPSSupport::LeftToRightBottomToTop;
        } else if (layout == QLatin1String("rltb")) {
            pagesPerSheetLayout = QCUPSSupport::RightToLeftTopToBottom;
        } else if (layout == QLatin1String("rlbt")) {
            pagesPerSheetLayout = QCUPSSupport::RightToLeftBottomToTop;
        } else if (layout == QLatin1String("tblr")) {
            pagesPerSheetLayout = QCUPSSupport::TopToBottomLeftToRight;
        } else if (layout == QLatin1String("tbrl")) {
            pagesPerSheetLayout = QCUPSSupport::TopToBottomRightToLeft;
        } else if (layout == QLatin1String("btlr")) {
            pagesPerSheetLayout = QCUPSSupport::BottomToTopLeftToRight;
        } else if (layout == QLatin1String("btrl")) {
            pagesPerSheetLayout = QCUPSSupport::BottomToTopRightToLeft;
        }
    }
    QCUPSSupport::setPagesPerSheetLayout(printer, pagesPerSheet, pagesPerSheetLayout);

    if (settings.contains(QLatin1String("output-bin"))) {
        // TODO not sure what this setting represents
    }
    if (settings.contains(QLatin1String("resolution-x"))) {
        // TODO possible to set only full resolution, but I can count the total
        // resolution I guess, anyway doesn't seem to be used by the print dialog
    }
    if (settings.contains(QLatin1String("resolution-y"))) {
        // TODO possible to set only full resolution, but I can count the total
        // resolution I guess, anyway doesn't seem to be used by the print dialog
    }
    if (settings.contains(QLatin1String("printer-lpi"))) {
        // TODO not possible to set, maybe count it?
    }
    if (settings.contains(QLatin1String("output-basename"))) {
        // TODO processed in output-uri
        // printer->setOutputFileName(settings.value(QLatin1String("output-basename")).toString());
    }
    if (settings.contains(QLatin1String("output-file-format"))) {
        // TODO only PDF supported by Qt so when printing to file we can use PDF only
        // btw. this should be already set automatically because we set output file name
        printer->setOutputFormat(QPrinter::PdfFormat);
    }
    if (settings.contains(QLatin1String("output-uri"))) {
        const QUrl uri = QUrl(settings.value(QLatin1String("output-uri")).toString());
        // Check whether the uri is not just a directory name and whether we don't need to
        // append output-basename
        if (settings.contains(QLatin1String("output-basename"))) {
            const QString basename = settings.value(QLatin1String("output-basename")).toString();
            if (!uri.toDisplayString().endsWith(basename) && uri.toDisplayString().endsWith(QLatin1Char('/'))) {
                printer->setOutputFileName(uri.toDisplayString() + basename);
            }
        } else {
            printer->setOutputFileName(uri.toDisplayString());
        }
    }

    // Process page setup
    QMarginsF pageMargins = printer->pageLayout().margins(QPageLayout::Millimeter);

    if (page_setup.contains(QLatin1String("PPDName"))) {
        printer->setPageSize(QPageSize(qt_idForPpdKey(page_setup.value(QLatin1String("PPDName")).toString())));
    }
    if (page_setup.contains(QLatin1String("Name"))) {
        // TODO: Try to use name instead of PPDName, at least I think it's how it's supposed to be used
        if (!page_setup.contains(QLatin1String("PPDName"))) {
            printer->setPageSize(QPageSize(qt_idForPpdKey(page_setup.value(QLatin1String("PPDName")).toString())));
        }
    }
    if (page_setup.contains(QLatin1String("DisplayName"))) {
        // TODO: This should just set a different name for the standardized one I guess
        printer->setPageSize(QPageSize(printer->pageLayout().pageSize().size(QPageSize::Millimeter),
                                         QPageSize::Millimeter, page_setup.value(QLatin1String("DisplayName")).toString()));
    }
    if (page_setup.contains(QLatin1String("Width")) && page_setup.contains(QLatin1String("Height"))) {
        QSizeF paperSize;
        paperSize.setHeight(page_setup.value(QLatin1String("Height")).toReal());
        paperSize.setWidth(page_setup.value(QLatin1String("Width")).toReal());
        printer->setPageSize(QPageSize(paperSize, QPageSize::Millimeter, page_setup.value(QLatin1String("DisplayName")).toString()));
    }
    if (page_setup.contains(QLatin1String("MarginTop"))) {
        pageMargins.setTop(page_setup.value(QLatin1String("MarginTop")).toReal());
    }
    if (page_setup.contains(QLatin1String("MarginBottom"))) {
        pageMargins.setBottom(page_setup.value(QLatin1String("MarginBottom")).toReal());
    }
    if (page_setup.contains(QLatin1String("MarginLeft"))) {
        pageMargins.setLeft(page_setup.value(QLatin1String("MarginLeft")).toReal());
    }
    if (page_setup.contains(QLatin1String("MarginRight"))) {
        pageMargins.setRight(page_setup.value(QLatin1String("MarginLeft")).toReal());
    }
    if (page_setup.contains(QLatin1String("Orientation"))) {
        const QString orientation = page_setup.value(QLatin1String("Orientation")).toString();
        if (orientation == QLatin1String("landscape") ||
            orientation == QLatin1String("reverse_landscape")) {
            printer->setPageOrientation(QPageLayout::Landscape);
        } else if (orientation == QLatin1String("portrait") ||
                   orientation == QLatin1String("reverse_portrait")) {
            printer->setPageOrientation(QPageLayout::Portrait);
        }
    }

    printer->setPageMargins(pageMargins, QPageLayout::Millimeter);

    QPrintDialog *printDialog = new QPrintDialog(printer);

    // Process options

    if (options.contains(QLatin1String("modal"))) {
        printDialog->setModal(options.value(QLatin1String("modal")).toBool());
    }

    // Pass back what we configured

    if (printDialog->exec() == QDialog::Accepted) {
        QVariantMap resultingSettings;
        QVariantMap resultingPageSetup;

        // Process back printer settings
        resultingSettings.insert(QLatin1String("n-copies"), QString::number(printer->copyCount()));
        resultingSettings.insert(QLatin1String("resolution"), QString::number(printer->resolution()));
        resultingSettings.insert(QLatin1String("use-color"), printer->colorMode() == QPrinter::Color ? QLatin1String("yes") : QLatin1String("no"));
        if (printer->duplex() == QPrinter::DuplexNone) {
            resultingSettings.insert(QLatin1String("duplex"), QLatin1String("simplex"));
        } else if (printer->duplex() == QPrinter::DuplexShortSide) {
            resultingSettings.insert(QLatin1String("duplex"), QLatin1String("horizontal"));
        } else if (printer->duplex() == QPrinter::DuplexLongSide) {
            resultingSettings.insert(QLatin1String("duplex"), QLatin1String("vertical"));
        }
        resultingSettings.insert(QLatin1String("collate"), printer->collateCopies() ? QLatin1String("yes") : QLatin1String("no"));
        resultingSettings.insert(QLatin1String("reverse"), printer->pageOrder() == QPrinter::LastPageFirst ? QLatin1String("yes") : QLatin1String("no"));
        if (printer->printRange() == QPrinter::AllPages) {
            resultingSettings.insert(QLatin1String("print-pages"), QLatin1String("all"));
        } else if (printer->printRange() == QPrinter::Selection) {
            resultingSettings.insert(QLatin1String("print-pages"), QLatin1String("selection"));
        } else if (printer->printRange() == QPrinter::CurrentPage) {
            resultingSettings.insert(QLatin1String("print-pages"), QLatin1String("current"));
        } else if (printer->printRange() == QPrinter::PageRange) {
            resultingSettings.insert(QLatin1String("print-pages"), QLatin1String("ranges"));
            resultingSettings.insert(QLatin1String("page-ranges"), QString("%1-%2").arg(printer->fromPage()).arg(printer->toPage()));
        }
        // Set cups specific properties
        const QStringList cupsOptions = QCUPSSupport::cupsOptionsList(printer);
        if (cupsOptions.contains(QLatin1String("page-set"))) {
            resultingSettings.insert(QLatin1String("page-set"), cupsOptions.at(cupsOptions.indexOf(QLatin1String("page-set")) + 1));
        }        if (cupsOptions.contains(QLatin1String("number-up"))) {
            resultingSettings.insert(QLatin1String("number-up"), cupsOptions.at(cupsOptions.indexOf(QLatin1String("number-up")) + 1));
        }
        if (cupsOptions.contains(QLatin1String("number-up-layout"))) {
            resultingSettings.insert(QLatin1String("number-up-layout"), cupsOptions.at(cupsOptions.indexOf(QLatin1String("number-up-layout")) + 1));
        }

        if (printer->outputFormat() == QPrinter::PdfFormat) {
            resultingSettings.insert(QLatin1String("output-file-format"), QLatin1String("pdf"));
        }

        if (!printer->outputFileName().isEmpty()) {
            resultingSettings.insert(QLatin1String("output-uri"), QUrl::fromLocalFile(printer->outputFileName()).toDisplayString());
        }

//         resultingSettings.insert(QLatin1String("printer"), QLatin1String("Canon_MG7500_series"));
//         resultingSettings.insert(QLatin1String("print-at"), QLatin1String("now"));
//         resultingSettings.insert(QLatin1String("scale"), QLatin1String("100"));
//         resultingSettings.insert(QLatin1String("cups-job-sheets"), QLatin1String("none,none"));
//         resultingSettings.insert(QLatin1String("cups-number-up"), QLatin1String("1"));
//         resultingSettings.insert(QLatin1String("cups-Resolution"), QLatin1String("601x600dpi"));
//         resultingSettings.insert(QLatin1String("cups-MediaType"), QLatin1String("Plain"));
//         resultingSettings.insert(QLatin1String("default-source"), QLatin1String("cassette"));
//         resultingSettings.insert(QLatin1String("cups-StpBrightness"), QLatin1String("None"));
//         resultingSettings.insert(QLatin1String("cups-Duplex"), QLatin1String("None"));
//         resultingSettings.insert(QLatin1String("cups-Resolution"), QLatin1String("601x600dpi"));
//         resultingSettings.insert(QLatin1String("cups-ColorModel"), QLatin1String("RGB"));
//         resultingSettings.insert(QLatin1String("dcups-StpQuality"), QLatin1String("Standard"));
//         resultingSettings.insert(QLatin1String("media-type"), QLatin1String("Plain"));
//         resultingSettings.insert(QLatin1String("cups-job-priority"), QLatin1String("50"));
//         resultingSettings.insert(QLatin1String("cups-StpContrast"), QLatin1String("None"));
//         resultingSettings.insert(QLatin1String("cups-StpInkType"), QLatin1String("CMYK"));

        // Process back page setup
        resultingPageSetup.insert(QLatin1String("PPDName"), qt_keyForPageSizeId(printer->pageLayout().pageSize().id()));
        // TODO: verify if this make sense
        resultingPageSetup.insert(QLatin1String("Name"), qt_keyForPageSizeId(printer->pageLayout().pageSize().id()));
        // TODO: verify if this make sense
        resultingPageSetup.insert(QLatin1String("DisplayName"), qt_keyForPageSizeId(printer->pageLayout().pageSize().id()));
        resultingPageSetup.insert(QLatin1String("Width"), printer->pageLayout().pageSize().size(QPageSize::Millimeter).width());
        resultingPageSetup.insert(QLatin1String("Height"), printer->pageLayout().pageSize().size(QPageSize::Millimeter).height());
        resultingPageSetup.insert(QLatin1String("MarginTop"), printer->pageLayout().margins(QPageLayout::Millimeter).top());
        resultingPageSetup.insert(QLatin1String("MarginBottom"),printer->pageLayout().margins(QPageLayout::Millimeter).bottom());
        resultingPageSetup.insert(QLatin1String("MarginLeft"), printer->pageLayout().margins(QPageLayout::Millimeter).left());
        resultingPageSetup.insert(QLatin1String("MarginRight"), printer->pageLayout().margins(QPageLayout::Millimeter).right());
        resultingPageSetup.insert(QLatin1String("Orientation"), printer->pageLayout().orientation() == QPageLayout::Landscape ? QLatin1String("landscape") : QLatin1String("portrait"));

        qCDebug(XdgDesktopPortalKdePrint) << "Settings: ";
        qCDebug(XdgDesktopPortalKdePrint) << "---------------------------";
        qCDebug(XdgDesktopPortalKdePrint) << resultingSettings;
        qCDebug(XdgDesktopPortalKdePrint) << "Page setup: ";
        qCDebug(XdgDesktopPortalKdePrint) << "---------------------------";
        qCDebug(XdgDesktopPortalKdePrint) << resultingPageSetup;

        uint token = QDateTime::currentDateTime().toTime_t();
        results.insert(QLatin1String("settings"), resultingSettings);
        results.insert(QLatin1String("page-setup"), resultingPageSetup);
        results.insert(QLatin1String("token"), token);

        m_printers.insert(token, printer);

        printDialog->deleteLater();
        return 0;
    } else {
        return 1;
    }

    return 0;
}
