/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>

class Session;
class DataControlManager;
class DataControlDevice;
class DataControlSource;

class ClipboardPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Clipboard")
public:
    explicit ClipboardPortal(QObject *parent);

    QVariant fetchData(Session *session, const QString &mimetype);

public Q_SLOTS:
    void RequestClipboard(const QDBusObjectPath &session_handle, const QVariantMap &options);
    void SetSelection(const QDBusObjectPath &session_handle, const QVariantMap &options);
    QDBusUnixFileDescriptor SelectionWrite(const QDBusObjectPath &session_handle, uint serial, const QDBusMessage &message);
    void SelectionWriteDone(const QDBusObjectPath &session_handle, uint serial, bool success, const QDBusMessage &message);
    QDBusUnixFileDescriptor SelectionRead(const QDBusObjectPath &session_handle, const QString &mime_type, const QDBusMessage &message);

private:
    void dataRequested(const DataControlSource &source, const QString &mimeType, int fd);

Q_SIGNALS:
    void SelectionOwnerChanged(const QDBusObjectPath &session_handle, const QVariantMap &options);
    void SelectionTransfer(const QDBusObjectPath &session_handle, const QString &mimeType, uint serial);

private:
    struct Transfer {
        uint serial = -1;
        int fd = -1;
    };
    std::vector<Transfer> m_pendingTransfers;
    std::unique_ptr<DataControlManager> m_dataControlManager;
    std::unique_ptr<DataControlDevice> m_dataControlDevice;
};
