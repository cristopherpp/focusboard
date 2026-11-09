#include "core/CommandController.hpp"
#include "core/DocumentStore.hpp"
#include "core/LayerShellWindowController.hpp"
#include "core/SettingsManager.hpp"
#include "integration/AutostartManager.hpp"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
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
    QElapsedTimer startup;
    startup.start();
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", QByteArrayLiteral("layer-shell"));
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Focusboard"));
    app.setOrganizationName(QStringLiteral("Focusboard"));
    app.setApplicationVersion(QStringLiteral(FOCUSBOARD_VERSION));
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
    const bool profileStartup = qEnvironmentVariableIsSet("FOCUSBOARD_PROFILE_STARTUP");
    if (profileStartup)
        qInfo("Focusboard bootstrap: %lld ms", startup.elapsed());
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
    if (profileStartup)
        qInfo("Focusboard window initialized: %lld ms", startup.elapsed());
    QSystemTrayIcon tray(
        QIcon::fromTheme(QStringLiteral("focusboard"), app.style()->standardIcon(QStyle::SP_DesktopIcon)));
    tray.setToolTip(QStringLiteral("Focusboard — Ctrl+K for commands"));
    auto showWindow = [window] {
        window->show();
        window->requestActivate();
    };
    auto hideWindow = [&] {
        const bool saved = documents.flush();
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            window->hide();
            if (!saved)
                tray.showMessage(QStringLiteral("Focusboard: unsaved changes"), documents.error(),
                                 QSystemTrayIcon::Warning);
        } else
            QMessageBox::information(nullptr, QStringLiteral("Focusboard"),
                                     QStringLiteral("The system tray is unavailable. Focusboard will stay "
                                                    "visible so you can reach it. Use :quit to exit."));
    };
    auto requestQuit = [&] {
        const bool documentsSaved = documents.flush();
        const bool settingsSaved = settings.sync();
        const QString failure = (documentsSaved ? QString() : documents.error()) +
                                (settingsSaved ? QString() : QStringLiteral("\nCannot save settings."));
        if ((!documentsSaved || !settingsSaved) &&
            QMessageBox::warning(nullptr, QStringLiteral("Focusboard: unsaved changes"),
                                 failure + QStringLiteral("\n\nQuit anyway?"),
                                 QMessageBox::Cancel | QMessageBox::Yes,
                                 QMessageBox::Cancel) != QMessageBox::Yes)
            return;
        app.quit();
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
    QObject::connect(menu.addAction(QStringLiteral("Quit")), &QAction::triggered, &app, requestQuit);
    tray.setContextMenu(&menu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, window,
                     [showWindow](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                             showWindow();
                     });
    QObject::connect(&commands, &CommandController::hideRequested, window, hideWindow);
    QObject::connect(&commands, &CommandController::quitRequested, &app, requestQuit);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &documents, [&] {
        if (!documents.flush())
            qCritical("Focusboard could not flush documents during shutdown: %s",
                      qPrintable(documents.error()));
        if (!settings.sync())
            qCritical("Focusboard could not save settings during shutdown.");
    });
    tray.show();
    if (!autostartReady)
        tray.showMessage(QStringLiteral("Autostart"), autostart.error(), QSystemTrayIcon::Warning);
    window->show();
    if (profileStartup)
        qInfo("Focusboard startup to first window show: %lld ms", startup.elapsed());
    return app.exec();
}
