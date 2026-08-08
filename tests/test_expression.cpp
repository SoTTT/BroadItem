#include <QtTest/QtTest>
#include <broaditem/expression/Expression.h>

using namespace BroadItem;

namespace {
/// @brief 将类型化分段压平为可比较的字符串列表（索引分段记为 "#n"）。
QStringList flatten(const BroadItem::Expression& expr)
{
    QStringList out;
    for (const auto& s : expr.segments())
        out << (s.isIndex() ? QStringLiteral("#%1").arg(s.index()) : s.key());
    return out;
}
} // namespace

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
        QCOMPARE(flatten(expr), QStringList({"device", "cpu"}));
    }

    void testParseBracketPath()
    {
        BroadItem::Expression expr("items[0].name");
        QVERIFY(expr.isValid());
        QCOMPARE(flatten(expr), QStringList({"items", "#0", "name"}));
    }

    void testParseMultiBracket()
    {
        BroadItem::Expression expr("a[0][1].c");
        QVERIFY(expr.isValid());
        QCOMPARE(flatten(expr), QStringList({"a", "#0", "#1", "c"}));
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
        QCOMPARE(flatten(expr), QStringList({"name"}));
    }

    void testParseTrailingBracket()
    {
        BroadItem::Expression expr("items[0]");
        QVERIFY(expr.isValid());
        QCOMPARE(flatten(expr), QStringList({"items", "#0"}));
    }

    // ---- 语法收紧（2026-08，统一语法权威后固定）----

    /// @brief ']' 后裸键非法（曾为解析期合法、运行期必报错的分歧点）。
    void testParseBareKeyAfterBracket()
    {
        BroadItem::Expression expr("a[0]b");
        QVERIFY(!expr.isValid());
    }

    /// @brief 结尾点号非法（含 ']' 后的；曾为解析期非法、运行期静默接受的分歧点）。
    void testParseTrailingDot()
    {
        QVERIFY(!BroadItem::Expression("a.").isValid());
        QVERIFY(!BroadItem::Expression("a[0].").isValid());
    }

    /// @brief 前导 '[' 与 '.[' 非法。
    void testParseLeadingBracket()
    {
        QVERIFY(!BroadItem::Expression("[0].a").isValid());
        QVERIFY(!BroadItem::Expression("a.[0]").isValid());
    }

    // ---- 键标识符规则（2026-08 收紧，doc/路径语法.md §3.1）----

    /// @brief 数字或数字开头的键非法。
    void testParseDigitLeadingKey()
    {
        QVERIFY(!BroadItem::Expression("123").isValid());
        QVERIFY(!BroadItem::Expression("1abc").isValid());
        QVERIFY(!BroadItem::Expression("a.2024.b").isValid());
    }

    /// @brief 键含连字符/空格/']' 等特殊字符非法。
    void testParseSpecialCharKey()
    {
        QVERIFY(!BroadItem::Expression("a-b").isValid());
        QVERIFY(!BroadItem::Expression("a b").isValid());
        QVERIFY(!BroadItem::Expression("a]b").isValid());
    }

    /// @brief 下划线开头、含数字、Unicode 字母键合法。
    void testParseUnicodeIdentKey()
    {
        QVERIFY(BroadItem::Expression("_private").isValid());
        QVERIFY(BroadItem::Expression("a1").isValid());
        BroadItem::Expression expr(QStringLiteral("设备.cpu"));
        QVERIFY(expr.isValid());
        QCOMPARE(flatten(expr), QStringList({QStringLiteral("设备"), "cpu"}));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestExpression)
#include "test_expression.moc"
