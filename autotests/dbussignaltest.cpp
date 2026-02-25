/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
    SPDX-FileCopyrightText: 2026 David Redondo <kde@david-redondo.de>
*/

#include "../src/dbushelpers.h"

#include <QTest>

using namespace Qt::StringLiterals;

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    SignalReceiver()
    {
    }
    void listen()
    {
        QDBusConnection::sessionBus().registerService(u"org.freedesktop.portal.Desktop"_s);
        QDBusConnection::sessionBus()
            .connect(QDBusConnection::sessionBus().baseService(), QString(), u"org.kde.testportal"_s, QString(), this, SLOT(signalArrived(QDBusMessage)));
    }
    QList<QDBusMessage> messages;
private Q_SLOTS:
    void signalArrived(const QDBusMessage &message)
    {
        messages << message;
    }
};

class DbusSignalTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testWrongCalls();
    void testSignals_data();
    void testSignals();

private:
    SignalReceiver signalReceiver;
};

class TestPortal : public QDBusAbstractAdaptor
{
    Q_CLASSINFO("D-Bus Interface", "org.kde.testportal")
    Q_OBJECT
public:
    void notASignal() {};
Q_SIGNALS:
    void signalVoid();
    void signalInt(int);
    void signalStringDouble(const QString &, double);
};

void DbusSignalTest::initTestCase()
{
    signalReceiver.listen();
}

void DbusSignalTest::testWrongCalls()
{
    auto canCallSendSignal = []<typename... T>(T &&...args) {
        return requires { sendSignal(std::forward<T>(args)...); };
    };
    // Cannot pass a random function
    static_assert(!canCallSendSignal(&rand));
    // Object needs to be a QDBusAbstractAdaptor
    static_assert(!canCallSendSignal(&QObject::parent));
    // Need to pass the correct amount of arguments
    static_assert(!canCallSendSignal(&TestPortal::signalVoid, 1));
    static_assert(!canCallSendSignal(&TestPortal::signalInt));
    // And need to be of correct types
    static_assert(!canCallSendSignal(&TestPortal::signalStringDouble, 0.2, true));
}

using functionCall = void (*)();

void DbusSignalTest::testSignals_data()
{
    QTest::addColumn<functionCall>("storedCall");
    QTest::addColumn<QString>("signalName");
    QTest::addColumn<QString>("signature");
    QTest::addColumn<QVariantList>("values");

    QTest::addRow("signalVoid") << +[]{sendSignal(&TestPortal::signalVoid);} << u"signalVoid"_s <<  u""_s << QVariantList{};
    QTest::addRow("signalInt") << +[]{sendSignal(&TestPortal::signalInt, 42);} << u"signalInt"_s <<  u"i"_s << QVariantList{42};
    QTest::addRow("signalStringDouble") << +[]{sendSignal(&TestPortal::signalStringDouble, u"hello!"_s, 33.8);} << u"signalStringDouble"_s <<  u"sd"_s << QVariantList{u"hello!"_s, 33.8};
}

void DbusSignalTest::testSignals()
{
    QFETCH(functionCall, storedCall);
    QFETCH(const QString, signalName);
    QFETCH(const QString, signature);
    QFETCH(const QVariantList, values);

    storedCall();

    QTRY_VERIFY(signalReceiver.messages.count());
    const QDBusMessage receivedSignal = signalReceiver.messages.takeFirst();

    QCOMPARE(receivedSignal.type(), QDBusMessage::SignalMessage);
    QCOMPARE(receivedSignal.path(), u"/org/freedesktop/portal/desktop"_s);
    QCOMPARE(receivedSignal.interface(), u"org.kde.testportal"_s);
    QCOMPARE(receivedSignal.member(), signalName);
    QCOMPARE(receivedSignal.signature(), signature);
    QCOMPARE(receivedSignal.arguments(), values);
}

QTEST_MAIN(DbusSignalTest)

#include "dbussignaltest.moc"
