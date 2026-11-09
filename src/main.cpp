#include "core/CommandController.hpp"
#include "core/DocumentStore.hpp"
#include "core/LayerShellWindowController.hpp"
#include "core/SettingsManager.hpp"
#include "integration/AutostartManager.hpp"
#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QMenu>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QStyle>
#include <QSystemTrayIcon>

int main(int argc, char **argv) {
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", QByteArrayLiteral("layer-shell"));
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Focusboard"));
    app.setOrganizationName(QStringLiteral("Focusboard"));
    app.setDesktopFileName(QStringLiteral("focusboard"));
    app.setQuitOnLastWindowClosed(false);
    if (!app.platformName().startsWith(QStringLiteral("wayland"))) {
        qCritical("Focusboard requires a Wayland session.");
        return 1;
    }
    const QString lockPath = QDir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation))
                                 .filePath(QStringLiteral("focusboard.lock"));
    QLockFile lock(lockPath);
    if (!lock.tryLock(0)) {
        qWarning("Focusboard is already running; use its tray icon to restore it.");
        return 0;
    }
    DocumentStore documents;
    SettingsManager settings;
    AutostartManager autostart;
    const bool autostartReady = autostart.migrateExistingEntry();
    LayerShellWindowController controller(&settings);
    CommandController commands(&documents, &settings);
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("documents"), QVariant::fromValue(&documents)},
                                 {QStringLiteral("settings"), QVariant::fromValue(&settings)},
                                 {QStringLiteral("layerController"), QVariant::fromValue(&controller)},
                                 {QStringLiteral("commands"), QVariant::fromValue(&commands)}});
    engine.loadFromModule(QStringLiteral("Focusboard"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window || !controller.initialize(window))
        return 1;
    QSystemTrayIcon tray(
        QIcon::fromTheme(QStringLiteral("focusboard"), app.style()->standardIcon(QStyle::SP_DesktopIcon)));
    tray.setToolTip(QStringLiteral("Focusboard — Ctrl+K for commands"));
    auto showWindow = [window] {
        window->show();
        window->requestActivate();
    };
    auto hideWindow = [&] {
        documents.flush();
        if (QSystemTrayIcon::isSystemTrayAvailable())
            window->hide();
        else
            QMessageBox::information(nullptr, QStringLiteral("Focusboard"),
                                     QStringLiteral("The system tray is unavailable. Focusboard will stay "
                                                    "visible so you can reach it. Use :quit to exit."));
    };
    QMenu menu;
    QObject::connect(menu.addAction(QStringLiteral("Show Focusboard")), &QAction::triggered, window,
                     showWindow);
    QObject::connect(menu.addAction(QStringLiteral("Hide Focusboard")), &QAction::triggered, window,
                     hideWindow);
    auto *login = menu.addAction(QStringLiteral("Launch at login"));
    login->setCheckable(true);
    login->setChecked(autostart.isEnabled());
    QObject::connect(login, &QAction::toggled, &autostart, [&](bool enabled) {
        if (autostart.setEnabled(enabled))
            settings.setAutostart(enabled);
        else {
            QSignalBlocker blocker(login);
            login->setChecked(autostart.isEnabled());
            tray.showMessage(QStringLiteral("Autostart"), autostart.error(), QSystemTrayIcon::Warning);
        }
    });
    menu.addSeparator();
    QObject::connect(menu.addAction(QStringLiteral("Quit")), &QAction::triggered, &app,
                     &QCoreApplication::quit);
    tray.setContextMenu(&menu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, window,
                     [showWindow](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                             showWindow();
                     });
    QObject::connect(&commands, &CommandController::hideRequested, window, hideWindow);
    QObject::connect(&commands, &CommandController::quitRequested, &app, &QCoreApplication::quit);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &documents, &DocumentStore::flush);
    tray.show();
    if (!autostartReady)
        tray.showMessage(QStringLiteral("Autostart"), autostart.error(), QSystemTrayIcon::Warning);
    window->show();
    return app.exec();
}
