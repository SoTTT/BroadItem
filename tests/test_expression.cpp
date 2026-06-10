#include <QtTest/QtTest>
#include "broaditem/Expression.h"

class TestExpression : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testParseEmptyPath()
    {
        BroadItem::Expression expr("");
        QVERIFY(!expr.isValid());
        QVERIFY(expr.segments().empty());
    }

    void testParseSimplePath()
    {
        BroadItem::Expression expr("device.cpu");
        QVERIFY(expr.isValid());
        QCOMPARE(expr.segments(), std::vector<QString>({"device", "cpu"}));
    }

    void testParseBracketPath()
    {
        BroadItem::Expression expr("items[0].name");
        QVERIFY(expr.isValid());
        QCOMPARE(expr.segments(), std::vector<QString>({"items", "0", "name"}));
    }

    void testParseMultiBracket()
    {
        BroadItem::Expression expr("a[0][1].c");
        QVERIFY(expr.isValid());
        QCOMPARE(expr.segments(), std::vector<QString>({"a", "0", "1", "c"}));
    }

    void testParseInvalidNegativeIndex()
    {
        BroadItem::Expression expr("items[-1]");
        QVERIFY(!expr.isValid());
    }

    void testParseDotOnly()
    {
        BroadItem::Expression expr("device.");
        QVERIFY(!expr.isValid());
    }

    void testParseDoubleDot()
    {
        BroadItem::Expression expr("a..b");
        QVERIFY(!expr.isValid());
    }

    void testParseUnmatchedBracket()
    {
        BroadItem::Expression expr("a[0");
        QVERIFY(!expr.isValid());
    }

    void testParseSimpleKey()
    {
        BroadItem::Expression expr("name");
        QVERIFY(expr.isValid());
        QCOMPARE(expr.segments(), std::vector<QString>({"name"}));
    }

    void testParseTrailingBracket()
    {
        BroadItem::Expression expr("items[0]");
        QVERIFY(expr.isValid());
        QCOMPARE(expr.segments(), std::vector<QString>({"items", "0"}));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestExpression)
#include "test_expression.moc"
