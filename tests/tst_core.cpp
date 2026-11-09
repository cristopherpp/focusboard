#include "../src/core/CommandController.hpp"
#include "../src/core/DocumentStore.hpp"
#include "../src/core/SettingsManager.hpp"
#include "../src/integration/AutostartManager.hpp"
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
class CoreTests : public QObject {
    Q_OBJECT
    QTemporaryDir environment;
    int testNumber = 0;
  private slots:
    void init() {
        const QString path = environment.path() + QStringLiteral("/%1").arg(++testNumber);
        QVERIFY(QDir().mkpath(path));
        qputenv("XDG_DATA_HOME", path.toUtf8());
        qputenv("XDG_CONFIG_HOME", path.toUtf8());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, path);
    }
    void zoomBounds() {
        SettingsManager s;
        s.setZoom(200);
        QCOMPARE(s.zoom(), 32);
        s.setZoom(-4);
        QCOMPARE(s.zoom(), 10);
    }
    void stateAndClamping() {
        SettingsManager s;
        s.setCompact(true);
        QVERIFY(s.compact());
        s.setCompact(false);
        QVERIFY(!s.compact());
        QCOMPARE(s.clampX(-5, 1000, 300), 0);
        QCOMPARE(s.clampX(900, 1000, 300), 700);
        QCOMPARE(s.clampY(800, 600, 300), 300);
        QCOMPARE(s.clampY(30, 40, 300), 0);
    }
    void markdownRoundTrip() {
        const QString text =
            QStringLiteral("# Goals\r\n- [ ] Study C++  \r\n- [x] Complete project\r\nπ 🦊\n\t`code`\n");
        {
            DocumentStore store;
            store.setDocument(QStringLiteral("goals"), text);
            store.setDocument(QStringLiteral("notes"), QStringLiteral("**Keep**\n"));
            store.flush();
            QVERIFY(store.error().isEmpty());
        }
        DocumentStore restored;
        QCOMPARE(restored.goals(), text);
        QCOMPARE(restored.notes(), QStringLiteral("**Keep**\n"));
        QFile f(restored.dataDirectory() + QStringLiteral("/goals.md"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), text.toUtf8());
    }
    void failedLoadIsNotOverwritten() {
        const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QVERIFY(QDir().mkpath(folder + QStringLiteral("/goals.md")));
        DocumentStore store;
        QVERIFY(!store.error().isEmpty());
        store.setDocument(QStringLiteral("goals"), QStringLiteral("replacement"));
        store.flush();
        QVERIFY(QFileInfo(folder + QStringLiteral("/goals.md")).isDir());
        QVERIFY(!store.error().isEmpty());
        QCOMPARE(store.goals(), QStringLiteral("replacement"));
    }
    void failedWritePreservesMemoryAndRetries() {
        DocumentStore store;
        const QString path = store.dataDirectory() + QStringLiteral("/goals.md");
        QVERIFY(QDir().mkpath(path));
        store.setDocument(QStringLiteral("goals"), QStringLiteral("pending"));
        store.flush();
        QVERIFY(!store.error().isEmpty());
        QCOMPARE(store.goals(), QStringLiteral("pending"));
        QVERIFY(QDir().rmdir(path));
        store.flush();
        QVERIFY(store.error().isEmpty());
        DocumentStore restored;
        QCOMPARE(restored.goals(), QStringLiteral("pending"));
    }
    void settingsPersist() {
        {
            SettingsManager a;
            a.setZoom(23);
            a.setTheme(QStringLiteral("dark"));
            a.setCompact(true);
            a.setWidth(500);
            a.setHeight(700);
            a.setCorner(QStringLiteral("bottom-left"));
            a.setTopMargin(90);
            a.setRightMargin(80);
        }
        SettingsManager b;
        QCOMPARE(b.zoom(), 23);
        QCOMPARE(b.theme(), QStringLiteral("dark"));
        QVERIFY(b.compact());
        QCOMPARE(b.width(), 500);
        QCOMPARE(b.height(), 700);
        QCOMPARE(b.corner(), QStringLiteral("bottom-left"));
        QCOMPARE(b.topMargin(), 90);
        QCOMPARE(b.rightMargin(), 80);
        b.setCompact(false);
        QCOMPARE(b.height(), 700);
    }
    void autostartFile() {
        AutostartManager a;
        QVERIFY(a.setEnabled(true));
        QVERIFY(a.isEnabled());
        QVERIFY(a.setEnabled(false));
        QVERIFY(!a.isEnabled());
    }
    void installedAutostartMigration() {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
                             QStringLiteral("/autostart/focusboard.desktop");
        AutostartManager installed(QStringLiteral("/usr/bin/focusboard"),
                                   QStringLiteral("/usr/bin/focusboard"));
        QVERIFY(installed.migrateExistingEntry());
        QVERIFY(!QFile::exists(path)); // Installing never opts the user into autostart.
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile entry(path);
        QVERIFY(entry.open(QIODevice::WriteOnly));
        entry.write("[Desktop "
                    "Entry]\nType=Application\nName=Focusboard\nExec=/home/user/project/build-release/"
                    "focusboard\nHidden=true\nX-GNOME-Autostart-enabled=false\nOnlyShowIn=KDE;\n");
        entry.close();
        QVERIFY(installed.migrateExistingEntry());
        QVERIFY(!installed.isEnabled());
        QVERIFY(entry.open(QIODevice::ReadOnly));
        const auto migrated = entry.readAll();
        entry.close();
        QVERIFY(migrated.contains("Exec=\"/usr/bin/focusboard\"\n"));
        QVERIFY(migrated.contains("TryExec=/usr/bin/focusboard\n"));
        QVERIFY(migrated.contains("Hidden=true\n"));
        QVERIFY(migrated.contains("OnlyShowIn=KDE;\n"));
        QVERIFY(!migrated.contains("build-release"));
        QCOMPARE(
            QDir(QFileInfo(path).absolutePath()).entryList({QStringLiteral("*.desktop")}, QDir::Files).size(),
            1);
        AutostartManager developer(QStringLiteral("/tmp/project/build/focusboard"),
                                   QStringLiteral("/usr/bin/focusboard"));
        QVERIFY(developer.migrateExistingEntry());
        QVERIFY(entry.open(QIODevice::ReadOnly));
        QCOMPARE(entry.readAll(), migrated);
        entry.close();
        QVERIFY(installed.setEnabled(true));
        QVERIFY(installed.isEnabled());
        QVERIFY(installed.setEnabled(false));
        QVERIFY(!QFile::exists(path));
    }
    void autostartExecutableEscaping() {
        const auto entry = AutostartManager::desktopEntry(QStringLiteral("/tmp/a b/100%/focusboard"));
        QVERIFY(entry.contains(QStringLiteral("Exec=\"/tmp/a b/100%%/focusboard\"\n")));
        QVERIFY(entry.contains(QStringLiteral("TryExec=/tmp/a b/100%/focusboard\n")));
    }
    void parsing_data() {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("valid");
        const QStringList valid = {":help",
                                   ":edit goals",
                                   "edit notes",
                                   ":goal add two  spaces",
                                   ":goal toggle 2",
                                   ":goal delete 1",
                                   ":zoom +",
                                   ":zoom -",
                                   ":zoom reset",
                                   ":theme dark",
                                   ":collapse",
                                   ":expand",
                                   ":position bottom-right",
                                   ":resize 280 260",
                                   ":hide",
                                   ":quit"};
        const QStringList invalid = {"",
                                     ":help extra",
                                     ":edit",
                                     ":edit secret",
                                     ":goal add",
                                     ":goal toggle 0",
                                     ":goal delete -1",
                                     ":goal toggle 2 extra",
                                     ":goal toggle 99999999999999",
                                     ":goal add one\ntwo",
                                     ":zoom huge",
                                     ":theme blue",
                                     ":position middle",
                                     ":resize 1 2",
                                     ":resize 400 500 extra",
                                     ":resize x 300",
                                     ":quit now",
                                     ":run rm"};
        for (const auto &s : valid)
            QTest::newRow(qPrintable(s)) << s << true;
        for (const auto &s : invalid)
            QTest::newRow(qPrintable(s)) << s << false;
    }
    void parsing() {
        QFETCH(QString, input);
        QFETCH(bool, valid);
        QCOMPARE(ParsedCommand::parse(input).error.isEmpty(), valid);
    }
    void checklistPreservesSource() {
        QString fencedError;
        QCOMPARE(Checklist::append(QStringLiteral("```\nexample"), QStringLiteral("task"), &fencedError),
                 QStringLiteral("```\nexample"));
        QVERIFY(!fencedError.isEmpty());
        const QString before = QStringLiteral(
            "# Heading\r\n\r\n- [ ] duplicate  \r\n```md\r\n- [ ] code\r\n```\r\n    - [ ] indented "
            "code\r\n* [X] duplicate\r\nparagraph ![image](https://example.invalid/image)\r\n");
        QString error;
        const auto toggled = Checklist::change(before, 2, false, &error);
        QVERIFY(error.isEmpty());
        QString expected = before;
        expected.replace(QStringLiteral("* [X] duplicate"), QStringLiteral("* [ ] duplicate"));
        QCOMPARE(toggled, expected);
        const auto deleted = Checklist::change(before, 1, true, &error);
        expected = before;
        expected.remove(expected.indexOf(QStringLiteral("- [ ] duplicate")),
                        QStringLiteral("- [ ] duplicate  \r\n").size());
        QCOMPARE(deleted, expected);
        QCOMPARE(Checklist::change(before, 3, false, &error), before);
        QVERIFY(!error.isEmpty());
        QCOMPARE(Checklist::append(QStringLiteral("text\r\n"), QStringLiteral("  spaces")),
                 QStringLiteral("text\r\n- [ ]   spaces\r\n"));
    }
    void dispatch() {
        DocumentStore d;
        SettingsManager s;
        CommandController c(&d, &s);
        QSignalSpy edit(&c, &CommandController::editRequested);
        QVERIFY(c.execute(QStringLiteral(":theme dark")).isEmpty());
        QCOMPARE(s.theme(), QStringLiteral("dark"));
        QVERIFY(!c.execute(QStringLiteral(":theme nonsense")).isEmpty());
        QCOMPARE(s.theme(), QStringLiteral("dark"));
        c.execute(QStringLiteral(":zoom reset"));
        c.execute(QStringLiteral(":zoom +"));
        QCOMPARE(s.zoom(), 17);
        c.execute(QStringLiteral(":zoom -"));
        QCOMPARE(s.zoom(), 16);
        c.execute(QStringLiteral(":collapse"));
        QVERIFY(s.compact());
        c.execute(QStringLiteral(":expand"));
        QVERIFY(!s.compact());
        c.execute(QStringLiteral(":position bottom-left"));
        c.execute(QStringLiteral(":resize 600 800"));
        QCOMPARE(s.corner(), QStringLiteral("bottom-left"));
        QCOMPARE(s.width(), 600);
        QCOMPARE(s.height(), 800);
        c.execute(QStringLiteral(":goal add repeated"));
        c.execute(QStringLiteral(":goal add repeated"));
        c.execute(QStringLiteral(":goal toggle 2"));
        QCOMPARE(d.goals(), QStringLiteral("- [ ] repeated\n- [x] repeated\n"));
        c.execute(QStringLiteral(":goal delete 1"));
        QCOMPARE(d.goals(), QStringLiteral("- [x] repeated\n"));
        c.execute(QStringLiteral(":edit notes"));
        QCOMPARE(edit.count(), 1);
        QCOMPARE(edit.at(0).at(0).toString(), QStringLiteral("notes"));
        QCOMPARE(c.complete(QStringLiteral(":edit n")), QStringLiteral(":edit notes"));
        QCOMPARE(c.history(0), QStringLiteral(":edit notes"));
        QVERIFY(!c.execute(QStringLiteral(":help")).isEmpty());
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "tst_core.moc"
