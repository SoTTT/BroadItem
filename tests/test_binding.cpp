#include <QtTest/QtTest>
#include <broaditem/expression/Binding.h>

using namespace BroadItem;

class TestBinding : public QObject {
    Q_OBJECT

private slots:
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void testBindingCreation()
    {
        Binding binding("b:content", "device.cpu");
        QCOMPARE(binding.attributeName(), QString("b:content"));
        QCOMPARE(binding.path(), QString("device.cpu"));
        QVERIFY(binding.isValid());
    }

    void testBindsPropertyExactMatch()
    {
        Binding binding("b:content", "device");
        QVERIFY(binding.bindsProperty("device"));
    }

    void testBindsPropertyDotPrefix()
    {
        Binding binding("b:content", "device.cpu");
        // "device.cpu" starts with "device." → prefix match
        QVERIFY(binding.bindsProperty("device"));
    }

    void testBindsPropertyBracketPrefix()
    {
        Binding binding("b:content", "items[0]");
        // "items[0]" starts with "items[" → prefix match
        QVERIFY(binding.bindsProperty("items"));
    }

    void testBindsPropertyNoMatch()
    {
        Binding binding("b:content", "device.cpu");
        // "cpu" is NOT a prefix of "device.cpu"
        QVERIFY(!binding.bindsProperty("cpu"));
    }

    void testBindsPropertyNoSubstringMatch()
    {
        Binding binding("b:content", "device");
        // "devices" is not a prefix of "device" and "device" does not start with "devices."
        QVERIFY(!binding.bindsProperty("devices"));
    }

    void testBindingExpression()
    {
        Binding binding("b:content", "items[0].name");
        const auto& expr = binding.expression();
        const auto segs = expr.segments();
        QCOMPARE(segs.size(), 3);
        QCOMPARE(segs.at(0), QString("items"));
        QCOMPARE(segs.at(1), QString("0"));
        QCOMPARE(segs.at(2), QString("name"));
    }

    void testBindingAttributeName()
    {
        Binding binding("b:content", "name");
        QCOMPARE(binding.attributeName(), QString("b:content"));
    }

    void testMultipleBindings()
    {
        std::vector<Binding> bindings;
        bindings.emplace_back("b:content", "title");
        bindings.emplace_back(":width", "size.w");
        bindings.emplace_back(":height", "size.h");

        QCOMPARE(bindings.size(), size_t(3));
        QCOMPARE(bindings[0].attributeName(), QString("b:content"));
        QCOMPARE(bindings[1].attributeName(), QString(":width"));
        QCOMPARE(bindings[2].attributeName(), QString(":height"));
        QCOMPARE(bindings[0].path(), QString("title"));
        QCOMPARE(bindings[1].path(), QString("size.w"));
        QCOMPARE(bindings[2].path(), QString("size.h"));
    }

    void testBindingEmptyPath()
    {
        Binding binding("b:content", "");
        QVERIFY(!binding.isValid());
        QVERIFY(!binding.bindsProperty("device"));
        QVERIFY(!binding.bindsProperty(""));
        QVERIFY(!binding.bindsProperty("b:content"));
    }
    // NOLINTEND(readability-convert-member-functions-to-static)
};

QTEST_MAIN(TestBinding)
#include "test_binding.moc"
