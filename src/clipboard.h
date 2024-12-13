/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
*/

#ifndef XDG_DESKTOP_PORTAL_KDE_CLIPBOARD_H
#define XDG_DESKTOP_PORTAL_KDE_CLIPBOARD_H

#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QEventLoop>

class Session;

class ClipboardPortal : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Clipboard")
public:
    explicit ClipboardPortal(QObject *parent);
    ~ClipboardPortal() override;

public:
    QByteArray fetchData(Session *session, const QString &mimetype);

public Q_SLOTS:
    void RequestClipboard(const QDBusObjectPath &session_handle, const QVariantMap &options);
    void SetSelection(const QDBusObjectPath &session_handle, const QVariantMap &options);
    QDBusUnixFileDescriptor SelectionWrite(const QDBusObjectPath &session_handle, uint serial, const QDBusMessage &message);
    void SelectionWriteDone(const QDBusObjectPath &session_handle, uint serial, bool success);
    QDBusUnixFileDescriptor SelectionRead(const QDBusObjectPath &session_handle, const QString &mime_type, const QDBusMessage &message);

Q_SIGNALS:
    void SelectionOwnerChanged(const QDBusObjectPath &session_handle, const QVariantMap &options);
    void SelectionTransfer(const QDBusObjectPath &session_handle, const QString &mimeType, uint serial);

private:
    struct Transfer {
        QEventLoop &loop;
        int fd = -1;
        QByteArray data;
    };
    std::map<int, Transfer> m_pendingTransfers;
};

#endif
