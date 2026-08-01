#include "CommandRegistry.h"

#include <QtTest/QtTest>
#include <algorithm>

class TestCommandRegistry : public QObject
{
    Q_OBJECT

private slots:
    void replacesCommandsById();
    void exposesDefaultCommands();
};

void TestCommandRegistry::replacesCommandsById()
{
    CommandRegistry registry;
    registry.registerCommand({"file.save", "Save", "old", "Ctrl+S"});
    registry.registerCommand({"file.save", "حفظ", "new", "Ctrl+S"});

    QCOMPARE(registry.commands().size(), 1);
    QCOMPARE(registry.command("file.save").title, QString("حفظ"));
    QVERIFY(registry.contains("file.save"));
}

void TestCommandRegistry::exposesDefaultCommands()
{
    const auto commands = CommandRegistry::defaultCommands();
    QVERIFY(!commands.isEmpty());
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "quick.open";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "project.test";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "code.quickFix" and command.shortcut == "Ctrl+.";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "code.format" and
            command.shortcut == "Shift+Alt+F";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "code.workspaceSymbols" and
            command.shortcut == "Ctrl+T";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "code.expandSelection" and
            command.shortcut == "Shift+Alt+Right";
    }));
    QVERIFY(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
        return command.id == "code.shrinkSelection" and
            command.shortcut == "Shift+Alt+Left";
    }));
}

QTEST_MAIN(TestCommandRegistry)
#include "TestCommandRegistry.moc"
