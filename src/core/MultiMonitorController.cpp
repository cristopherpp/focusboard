#include "MultiMonitorController.hpp"

#include "AutostartManager.hpp"
#include "CommandController.hpp"
#include "DesktopWindowController.hpp"
#include "DocumentStore.hpp"
#include "SettingsManager.hpp"

#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QVariantMap>

struct MultiMonitorController::Surface {
    QPointer<QScreen> screen;
    QPointer<QQuickWindow> window;
    DesktopWindowController *desktopController = nullptr;
};

MultiMonitorController::MultiMonitorController(QQmlEngine *engine, DocumentStore *documents,
                                               SettingsManager *settings, CommandController *commands,
                                               AutostartManager *autostart, bool offerAutostart,
                                               QObject *parent)
    : QObject(parent), m_engine(engine), m_documents(documents), m_settings(settings), m_commands(commands),
      m_autostart(autostart), m_offerAutostart(offerAutostart) {}

MultiMonitorController::~MultiMonitorController() {
    const auto keys = m_surfaces.keys();
    for (const QString &key : keys)
        destroySurface(key);
}

bool MultiMonitorController::start() {
    if (m_started)
        return surfaceCount() > 0;
    m_started = true;
    auto *application = qGuiApp;
    connect(application, &QGuiApplication::screenAdded, this,
            [this](QScreen *) { QTimer::singleShot(0, this, &MultiMonitorController::synchronizeScreens); });
    connect(application, &QGuiApplication::screenRemoved, this,
            [this](QScreen *) { QTimer::singleShot(0, this, &MultiMonitorController::synchronizeScreens); });
    connect(application, &QGuiApplication::primaryScreenChanged, this,
            [this](QScreen *) { QTimer::singleShot(0, this, &MultiMonitorController::synchronizeScreens); });
    synchronizeScreens();
    return surfaceCount() > 0;
}

QString MultiMonitorController::screenKey(QScreen *screen) {
    if (!screen)
        return {};
    // Include the object identity so equal connector labels still represent
    // distinct outputs, while repeated screenAdded notifications stay idempotent.
    return screen->name() + QLatin1Char('@') + QString::number(reinterpret_cast<quintptr>(screen), 16);
}

QStringList MultiMonitorController::uniqueScreenNames(QStringList names) {
    names.removeAll(QString());
    names.removeDuplicates();
    names.sort();
    return names;
}

MultiMonitorController::ScreenDelta MultiMonitorController::screenDelta(const QStringList &current,
                                                                        const QStringList &connected) {
    const QStringList oldNames = uniqueScreenNames(current);
    const QStringList newNames = uniqueScreenNames(connected);
    ScreenDelta result;
    for (const QString &name : newNames)
        if (!oldNames.contains(name))
            result.added.append(name);
    for (const QString &name : oldNames)
        if (!newNames.contains(name))
            result.removed.append(name);
    return result;
}

QUrl MultiMonitorController::mainQmlUrl() {
    return QUrl(QStringLiteral("qrc:/qt/qml/Focusboard/qml/Main.qml"));
}

void MultiMonitorController::synchronizeScreens() {
    auto *application = qGuiApp;
    const auto screens = application->screens();
    QStringList connected;
    for (QScreen *screen : screens)
        connected.append(screenKey(screen));

    const ScreenDelta delta = screenDelta(m_surfaces.keys(), connected);
    for (const QString &key : delta.removed)
        destroySurface(key);

    // Create the primary monitor first so the one-time setup prompt, if needed,
    // is shown on the screen the user considers their main desktop.
    QList<QScreen *> ordered = screens;
    QScreen *primary = application->primaryScreen();
    if (primary && ordered.removeOne(primary))
        ordered.prepend(primary);
    for (QScreen *screen : ordered) {
        const QString key = screenKey(screen);
        if (!key.isEmpty() && !m_surfaces.contains(key))
            createSurface(screen);
    }
}

bool MultiMonitorController::createSurface(QScreen *screen) {
    if (!screen)
        return false;
    const QString key = screenKey(screen);
    if (key.isEmpty() || m_surfaces.contains(key))
        return false;

    auto *surface = new Surface;
    surface->screen = screen;
    surface->desktopController = new DesktopWindowController(m_settings, this);

    QQmlComponent component(m_engine, mainQmlUrl());
    if (!QFile::exists(QStringLiteral(":/qt/qml/Focusboard/qml/Main.qml"))) {
        qCritical().noquote() << "Focusboard QML resource is missing:" << mainQmlUrl();
        delete surface->desktopController;
        delete surface;
        return false;
    }
    if (component.status() != QQmlComponent::Ready) {
        qCritical().noquote() << "Focusboard Main.qml failed to compile:" << component.errors();
        delete surface->desktopController;
        delete surface;
        return false;
    }
    const QVariantMap properties{
        {QStringLiteral("documents"), QVariant::fromValue(m_documents)},
        {QStringLiteral("settings"), QVariant::fromValue(m_settings)},
        {QStringLiteral("commands"), QVariant::fromValue(m_commands)},
        {QStringLiteral("desktopController"), QVariant::fromValue(surface->desktopController)},
        {QStringLiteral("autostartManager"), QVariant::fromValue(m_autostart)},
        {QStringLiteral("offerAutostart"), m_offerAutostart && screen == qGuiApp->primaryScreen()},
    };
    QObject *object = component.createWithInitialProperties(properties);
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        qCritical().noquote() << "Focusboard could not instantiate compiled Main.qml on" << key
                              << component.errors();
        delete object;
        delete surface->desktopController;
        delete surface;
        return false;
    }
    window->QObject::setParent(this);
    surface->window = window;
    if (!surface->desktopController->initialize(window, screen)) {
        qCritical().noquote() << "Focusboard could not initialize its desktop window on" << key
                              << "(check platform integration and output availability).";
        delete window;
        delete surface->desktopController;
        delete surface;
        return false;
    }

    m_surfaces.insert(key, surface);
    if (m_offerAutostart && screen == qGuiApp->primaryScreen())
        m_offerAutostart = false;
    if (m_visible)
        window->show();
    return true;
}

void MultiMonitorController::destroySurface(const QString &key) {
    Surface *surface = m_surfaces.take(key);
    if (!surface)
        return;
    if (surface->window) {
        surface->window->hide();
        delete surface->window;
    }
    delete surface->desktopController;
    delete surface;
}

QList<QQuickWindow *> MultiMonitorController::windows() const {
    QList<QQuickWindow *> result;
    for (Surface *surface : m_surfaces)
        if (surface->window)
            result.append(surface->window);
    return result;
}

QQuickWindow *MultiMonitorController::primaryWindow() const {
    QScreen *primary = qGuiApp->primaryScreen();
    const auto it = m_surfaces.constFind(screenKey(primary));
    if (it != m_surfaces.cend())
        return (*it)->window;
    const auto allWindows = windows();
    return allWindows.isEmpty() ? nullptr : allWindows.constFirst();
}

int MultiMonitorController::surfaceCount() const { return m_surfaces.size(); }

bool MultiMonitorController::isVisible() const { return m_visible; }

void MultiMonitorController::showAll(bool activate) {
    m_visible = true;
    for (Surface *surface : m_surfaces)
        if (surface->window) {
            if (surface->window->windowState() == Qt::WindowMinimized)
                surface->window->showNormal();
            else
                surface->window->show();
        }
    if (activate)
        if (QQuickWindow *window = primaryWindow())
            window->requestActivate();
}

void MultiMonitorController::hideAll() {
    m_visible = false;
    for (Surface *surface : m_surfaces)
        if (surface->window)
            surface->window->hide();
}
