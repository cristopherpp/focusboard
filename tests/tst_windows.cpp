#include "../src/core/DesktopWindowController.hpp"
#include "../src/core/SettingsManager.hpp"
#include "../src/integration/AutostartManager.hpp"
#include <QGuiApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

class WindowsTests : public QObject {
    Q_OBJECT
    QTemporaryDir temporary;
  private slots:
    void initTestCase() {
        QVERIFY(temporary.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    }
    void nativeRegistration() {
        // Isolated disposable registry key; never the user's actual Run key.
        const QString key = QStringLiteral("HKEY_CURRENT_USER\\Software\\FocusboardTests\\") +
                            QUuid::createUuid().toString(QUuid::WithoutBraces);
        struct Cleanup {
            QString key;
            ~Cleanup() {
                QSettings registry(key, QSettings::NativeFormat);
                registry.remove(QString());
                registry.sync();
            }
        } cleanup{key};
        AutostartManager manager(QStringLiteral("C:/A path/日本/Focusboard.exe"), key);
        QVERIFY(!manager.hasConfiguration());
        QVERIFY(manager.setEnabled(true));
        QVERIFY(manager.isEnabled());
        QSettings registration(key, QSettings::NativeFormat);
        QCOMPARE(registration.allKeys(), QStringList{QStringLiteral("Focusboard")});
        QCOMPARE(registration.value(QStringLiteral("Focusboard")).toString(),
                 QStringLiteral("\"C:\\A path\\日本\\Focusboard.exe\""));
        QVERIFY(manager.setEnabled(false));
        registration.sync();
        QVERIFY(!registration.contains(QStringLiteral("Focusboard")));
    }
    void hiddenNativeWindow() {
        QVERIFY(QGuiApplication::platformName() == QStringLiteral("windows"));
        QScreen *screen = QGuiApplication::primaryScreen();
        QVERIFY(screen);
        SettingsManager settings;
        QQuickWindow first, second;
        DesktopWindowController firstController(&settings), secondController(&settings);
        QVERIFY(firstController.initialize(&first, screen));
        QVERIFY(secondController.initialize(&second, screen));
        const HWND hwnd = reinterpret_cast<HWND>(first.winId());
        const auto extended = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        QVERIFY(extended & WS_EX_TOOLWINDOW);
        QVERIFY(!(extended & WS_EX_APPWINDOW));
        QVERIFY(!(extended & WS_EX_TOPMOST));
        QVERIFY(!(extended & WS_EX_NOACTIVATE));
        QVERIFY(!(GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_CAPTION));
        QVERIFY(!IsWindowVisible(hwnd));
        const QRect original = second.geometry();
        firstController.applySize(400, 500);
        QCOMPARE(second.geometry(), original);
        firstController.beginDrag(0, 0);
        firstController.dragTo(-10000, -10000);
        QVERIFY(screen->availableGeometry().contains(first.geometry()));
        QCOMPARE(second.geometry(), original);
        const int expandedHeight = first.height();
        settings.setCompact(true);
        QCOMPARE(first.height(), qMin(84, screen->availableGeometry().height()));
        settings.setCompact(false);
        QCOMPARE(first.height(), expandedHeight);
        // HWND tests do not establish Show Desktop, shell or virtual-desktop behavior.
    }
};
QTEST_MAIN(WindowsTests)
#include "tst_windows.moc"
