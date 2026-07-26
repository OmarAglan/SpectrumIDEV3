#include "LspMessageFramer.h"

#include <QTest>

class TestLspMessageFramer : public QObject
{
    Q_OBJECT
private slots:
    void handlesUtf8PartialAndMultipleMessages();
    void rejectsDuplicateLength();
};

void TestLspMessageFramer::handlesUtf8PartialAndMultipleMessages()
{
    const QByteArray first = QStringLiteral("{\"اسم\":\"باء\"}").toUtf8();
    const QByteArray second = R"({"jsonrpc":"2.0"})";
    const QByteArray stream = LspMessageFramer::frame(first) + LspMessageFramer::frame(second);

    LspMessageFramer framer;
    QString error;
    QVERIFY(framer.appendData(stream.left(11), &error).isEmpty());
    QVERIFY(error.isEmpty());
    QCOMPARE(framer.appendData(stream.mid(11), &error),
             (QList<QByteArray>{first, second}));
    QVERIFY(error.isEmpty());
}

void TestLspMessageFramer::rejectsDuplicateLength()
{
    LspMessageFramer framer;
    QString error;
    const QList<QByteArray> messages = framer.appendData(
        "Content-Length: 2\r\nContent-Length: 2\r\n\r\n{}", &error);
    QVERIFY(messages.isEmpty());
    QVERIFY(not error.isEmpty());
}

QTEST_MAIN(TestLspMessageFramer)
#include "TestLspMessageFramer.moc"
