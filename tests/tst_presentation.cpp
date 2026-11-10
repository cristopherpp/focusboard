#include "../src/core/CommandController.hpp"
#include "../src/core/DesktopWindowController.hpp"
#include "../src/core/DocumentStore.hpp"
#include "../src/core/MarkdownPreview.hpp"
#include "../src/core/SettingsManager.hpp"
#include "../src/integration/AutostartManager.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#ifdef Q_OS_WIN
#include <QQuickStyle>
#endif
#include <QTemporaryDir>
#include <QtTest>

class PresentationTests : public QObject {
    Q_OBJECT
    QTemporaryDir environment;
  private slots:
    void initTestCase() {
#ifdef Q_OS_WIN
        QQuickStyle::setStyle(QStringLiteral("Fusion"));
#endif
        qputenv("XDG_DATA_HOME", environment.path().toUtf8());
        qputenv("XDG_CONFIG_HOME", environment.path().toUtf8());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, environment.path());
        qmlRegisterType<MarkdownPreview>("Focusboard", 1, 0, "MarkdownPreview");
        qmlRegisterUncreatableType<DocumentStore>("Focusboard", 1, 0, "DocumentStore", "app owned");
        qmlRegisterUncreatableType<SettingsManager>("Focusboard", 1, 0, "SettingsManager", "app owned");
        qmlRegisterUncreatableType<CommandController>("Focusboard", 1, 0, "CommandController", "app owned");
        qmlRegisterUncreatableType<DesktopWindowController>("Focusboard", 1, 0, "DesktopWindowController",
                                                            "app owned");
        qmlRegisterUncreatableType<AutostartManager>("Focusboard", 1, 0, "AutostartManager", "app owned");
    }
    void resourcesAreBlocked() {
        QImage original(40, 40, QImage::Format_ARGB32);
        original.fill(Qt::red);
        const QString path = environment.path() + "/private.png";
        QVERIFY(original.save(path));
        LocalMarkdownDocument document;
        const auto local =
            document.resource(QTextDocument::ImageResource, QUrl::fromLocalFile(path)).value<QImage>();
        QCOMPARE(local.size(), QSize(1, 1));
        QCOMPARE(local.pixelColor(0, 0).alpha(), 0);
        const auto remote =
            document.resource(QTextDocument::ImageResource, QUrl("https://example.invalid/image.png"))
                .value<QImage>();
        QCOMPARE(remote.size(), QSize(1, 1));
    }
    void keyboardPreviewEditAndCommands() {
        DocumentStore documents(environment.filePath(QStringLiteral("documents")));
        SettingsManager settings;
        CommandController commands(&documents, &settings);
        DesktopWindowController controller(&settings);
#ifdef Q_OS_WIN
        AutostartManager autostart(QStringLiteral("C:/Test/Focusboard.exe"),
                                   environment.filePath(QStringLiteral("run.ini")), QSettings::IniFormat);
#else
        AutostartManager autostart(QStringLiteral("/usr/bin/focusboard"),
                                   QStringLiteral("/usr/bin/focusboard"));
#endif
        documents.setDocument("goals", "# Original\n- [ ] one\n");
        QQmlEngine engine;
        QSignalSpy warnings(&engine, &QQmlEngine::warnings);
        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(QStringLiteral(FOCUSBOARD_SOURCE_DIR "/qml/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({{"documents", QVariant::fromValue(&documents)},
                                                   {"settings", QVariant::fromValue(&settings)},
                                                   {"commands", QVariant::fromValue(&commands)},
                                                   {"desktopController", QVariant::fromValue(&controller)},
                                                   {"autostartManager", QVariant::fromValue(&autostart)},
                                                   {"offerAutostart", false}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto *window = qobject_cast<QQuickWindow *>(object.get());
        QVERIFY(window);
        window->resize(360, 440);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        window->requestActivate();
        QTRY_VERIFY(window->isActive());
        auto *palette = object->findChild<QObject *>("commandPalette");
        QVERIFY(palette);
        auto *input = object->findChild<QObject *>("commandInput");
        QVERIFY(input);
        QTest::keyClick(window, Qt::Key_K, Qt::ControlModifier);
        QTRY_VERIFY(palette->property("visible").toBool());
        input->setProperty("text", ":edit goals");
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_COMPARE(object->property("editing").toString(), QString("goals"));
        auto *editor = object->findChild<QObject *>("markdownEditor");
        QVERIFY(editor);
        QTRY_VERIFY(editor->property("activeFocus").toBool());
        QTest::keyClick(window, Qt::Key_End, Qt::ControlModifier);
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(documents.goals().endsWith("x"));
        QTest::keyClick(window, Qt::Key_Return, Qt::ControlModifier);
        QTRY_COMPARE(object->property("editing").toString(), QString());
        DocumentStore restored(environment.filePath(QStringLiteral("documents")));
        QCOMPARE(restored.goals(), documents.goals());
        // A literal colon opens the palette only in preview mode.
        QTest::keyClick(window, Qt::Key_Colon);
        QTRY_VERIFY(palette->property("visible").toBool());
        input->setProperty("text", ":theme d");
        QTest::keyClick(window, Qt::Key_Tab);
        QCOMPARE(input->property("text").toString(), QString(":theme dark"));
        QTest::keyClick(window, Qt::Key_Return);
        QCOMPARE(settings.theme(), QString("dark"));
        QTest::keyClick(window, Qt::Key_K, Qt::ControlModifier);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(input->property("text").toString(), QString(":theme dark"));
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!palette->property("visible").toBool());
        QCOMPARE(warnings.count(), 0);
    }
    void firstLaunchChoices() {
        for (const bool enable : {true, false}) {
            QSettings raw(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral("Focusboard"),
                          QStringLiteral("Focusboard"));
            raw.remove(QStringLiteral("autostart"));
            raw.sync();
            const QString entry = environment.path() + QStringLiteral("/autostart/focusboard.desktop");
            QFile::remove(entry);
            QFile::remove(environment.filePath(QStringLiteral("run.ini")));
            DocumentStore documents(environment.filePath(QStringLiteral("documents")));
            SettingsManager settings;
#ifdef Q_OS_WIN
            AutostartManager autostart(QStringLiteral("C:/Test/Focusboard.exe"),
                                       environment.filePath(QStringLiteral("run.ini")), QSettings::IniFormat);
#else
            AutostartManager autostart(QStringLiteral("/usr/bin/focusboard"),
                                       QStringLiteral("/usr/bin/focusboard"));
#endif
            CommandController commands(&documents, &settings);
            DesktopWindowController controller(&settings);
            QQmlEngine engine;
            QQmlComponent component(
                &engine, QUrl::fromLocalFile(QStringLiteral(FOCUSBOARD_SOURCE_DIR "/qml/Main.qml")));
            QVERIFY2(component.isReady(), qPrintable(component.errorString()));
            std::unique_ptr<QObject> object(component.createWithInitialProperties(
                {{"documents", QVariant::fromValue(&documents)},
                 {"settings", QVariant::fromValue(&settings)},
                 {"commands", QVariant::fromValue(&commands)},
                 {"desktopController", QVariant::fromValue(&controller)},
                 {"autostartManager", QVariant::fromValue(&autostart)},
                 {"offerAutostart", true}}));
            QVERIFY2(object, qPrintable(component.errorString()));
            auto *window = qobject_cast<QQuickWindow *>(object.get());
            QVERIFY(window);
            window->resize(360, 440);
            window->show();
            QVERIFY(QTest::qWaitForWindowExposed(window));
            auto *popup = object->findChild<QObject *>("autostartPrompt");
            QVERIFY(popup);
            QTRY_VERIFY(popup->property("visible").toBool());
            auto *button = qobject_cast<QQuickItem *>(
                object->findChild<QObject *>(enable ? "autostartYes" : "autostartNo"));
            QVERIFY(button);
            const QPointF center = button->mapToScene(QPointF(button->width() / 2, button->height() / 2));
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
            QTRY_VERIFY(!popup->property("visible").toBool());
            QVERIFY(settings.hasAutostartPreference());
            QCOMPARE(settings.autostart(), enable);
            QCOMPARE(autostart.isEnabled(), enable);
#ifndef Q_OS_WIN
            QCOMPARE(QFile::exists(entry), enable);
#endif
        }
    }
};
QTEST_MAIN(PresentationTests)
#include "tst_presentation.moc"
