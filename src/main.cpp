#include "core/CommandController.hpp"
#include "core/DocumentStore.hpp"
#include "core/MultiMonitorController.hpp"
#include "core/SettingsManager.hpp"
#include "integration/AutostartManager.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLockFile>
#include <QMenu>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QStyle>
#include <QSystemTrayIcon>
#include <cstdio>

namespace {
int verifyQmlResource(int argc, char **argv) {
    QApplication app(argc, argv);
    QQmlEngine engine;
    const QUrl url = MultiMonitorController::mainQmlUrl();
    QFile resource(QStringLiteral(":/qt/qml/Focusboard/qml/Main.qml"));
    if (!resource.open(QIODevice::ReadOnly)) {
        qCritical().noquote() << "Embedded QML resource is missing at" << url;
        return 2;
    }
    QQmlComponent component(&engine, url);
    if (component.status() != QQmlComponent::Ready) {
        qCritical().noquote() << "Embedded Main.qml was found but failed to compile:" << component.errors();
        return 3;
    }
    return 0;
}

int runAutostartCommand(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Focusboard"));
    app.setOrganizationName(QStringLiteral("Focusboard"));
    SettingsManager settings;
    AutostartManager autostart;
    if (!autostart.migrateExistingEntry()) {
        qCritical("%s", qPrintable(autostart.error()));
        return 2;
    }
    const QString command = QString::fromLocal8Bit(argv[1]);
    if (command == QStringLiteral("--autostart-state")) {
        const char *state = autostart.hasConfiguration() || settings.hasAutostartPreference()
                                ? (autostart.isEnabled() ? "enabled" : "disabled")
                                : "unset";
        std::puts(state);
        return 0;
    }
    const bool enabled = command == QStringLiteral("--autostart-enable");
    if (!autostart.setEnabled(enabled)) {
        qCritical("%s", qPrintable(autostart.error()));
        return 2;
    }
    settings.setAutostart(enabled);
    if (!settings.sync()) {
        qCritical("Cannot save autostart preference.");
        return 2;
    }
    std::puts(enabled ? "enabled" : "disabled");
    return 0;
}
} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--verify-qml-resource"))
        return verifyQmlResource(argc, argv);
    if (argc == 2) {
        const QString command = QString::fromLocal8Bit(argv[1]);
        if (command == QStringLiteral("--autostart-state") ||
            command == QStringLiteral("--autostart-enable") ||
            command == QStringLiteral("--autostart-disable"))
            return runAutostartCommand(argc, argv);
    }
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
    const bool offerAutostart = autostartReady && autostart.isInstalled() && !autostart.hasConfiguration() &&
                                !settings.hasAutostartPreference();
    CommandController commands(&documents, &settings);
    QQmlApplicationEngine engine;
    MultiMonitorController monitors(&engine, &documents, &settings, &commands, &autostart, offerAutostart);
    if (!monitors.start()) {
        qCritical("Focusboard could not create a layer-shell surface for any connected monitor.");
        return 1;
    }
    const bool profileStartup = qEnvironmentVariableIsSet("FOCUSBOARD_PROFILE_STARTUP");
    if (profileStartup)
        qInfo("Focusboard bootstrap: %lld ms", startup.elapsed());
    if (profileStartup)
        qInfo("Focusboard window initialized: %lld ms", startup.elapsed());
    QSystemTrayIcon tray(
        QIcon::fromTheme(QStringLiteral("focusboard"), app.style()->standardIcon(QStyle::SP_DesktopIcon)));
    tray.setToolTip(QStringLiteral("Focusboard — Ctrl+K for commands"));
    auto showWindow = [&monitors] { monitors.showAll(true); };
    auto hideWindow = [&] {
        const bool saved = documents.flush();
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            monitors.hideAll();
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
    QObject::connect(menu.addAction(QStringLiteral("Show Focusboard")), &QAction::triggered, &app,
                     showWindow);
    QObject::connect(menu.addAction(QStringLiteral("Hide Focusboard")), &QAction::triggered, &app,
                     hideWindow);
    auto *login = menu.addAction(QStringLiteral("Launch at login"));
    login->setCheckable(true);
    login->setChecked(autostart.isEnabled());
    QObject::connect(&autostart, &AutostartManager::stateChanged, login, [&] {
        QSignalBlocker blocker(login);
        login->setChecked(autostart.isEnabled());
    });
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
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app,
                     [showWindow](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                             showWindow();
                     });
    QObject::connect(&commands, &CommandController::hideRequested, &app, hideWindow);
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
    if (profileStartup)
        qInfo("Focusboard startup to first window show: %lld ms", startup.elapsed());
    return app.exec();
}
