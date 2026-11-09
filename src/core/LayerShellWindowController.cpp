#include "LayerShellWindowController.hpp"
#include "SettingsManager.hpp"
#include <LayerShellQt/Window>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
LayerShellWindowController::LayerShellWindowController(SettingsManager *s, QObject *p)
    : QObject(p), m_settings(s) {}
bool LayerShellWindowController::initialize(QQuickWindow *window) {
    if (m_window)
        return m_window == window;
    m_window = window;
    m_layer = LayerShellQt::Window::get(window);
    if (!m_layer)
        return false;
    m_layer->setScope(QStringLiteral("focusboard"));
    m_layer->setLayer(LayerShellQt::Window::LayerBottom);
    m_layer->setExclusiveZone(0);
    m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    m_layer->setActivateOnShow(false);
    m_layer->setCloseOnDismissed(false);
    connect(m_settings, &SettingsManager::positionChanged, this, &LayerShellWindowController::updateGeometry);
    connect(m_settings, &SettingsManager::sizeChanged, this, &LayerShellWindowController::updateGeometry);
    connect(m_settings, &SettingsManager::compactChanged, this, &LayerShellWindowController::updateGeometry);
    connect(window, &QWindow::screenChanged, this, [this](QScreen *s) {
        watchScreen(s);
        updateGeometry();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] {
        QTimer::singleShot(0, this, [this] {
            if (!m_window)
                return;
            const bool visible = m_window->isVisible();
            m_window->hide();
            m_layer->setScreen(QGuiApplication::primaryScreen());
            m_window->setScreen(QGuiApplication::primaryScreen());
            updateGeometry();
            if (visible)
                m_window->show();
        });
    });
    watchScreen(window->screen());
    updateGeometry();
    return true;
}
void LayerShellWindowController::watchScreen(QScreen *screen) {
    if (!screen)
        return;
    connect(screen, &QScreen::availableGeometryChanged, this, &LayerShellWindowController::updateGeometry,
            Qt::UniqueConnection);
    connect(screen, &QScreen::geometryChanged, this, &LayerShellWindowController::updateGeometry,
            Qt::UniqueConnection);
    connect(screen, &QScreen::logicalDotsPerInchChanged, this, &LayerShellWindowController::updateGeometry,
            Qt::UniqueConnection);
}
void LayerShellWindowController::updateGeometry() {
    if (!m_window || !m_layer || !m_window->screen())
        return;
    const QSize available = m_window->screen()->availableGeometry().size();
    const QSize size(
        qMin(m_settings->width(), available.width()),
        qMin(m_settings->compact() && !m_paletteOpen ? 84 : m_settings->height(), available.height()));
    m_window->resize(size);
    m_layer->setDesiredSize(size);
    const QString corner = m_settings->corner();
    const bool left = corner.endsWith(QStringLiteral("left")), top = corner.startsWith(QStringLiteral("top"));
    using W = LayerShellQt::Window;
    m_layer->setAnchors(W::Anchors(left ? W::AnchorLeft : W::AnchorRight) |
                        W::Anchors(top ? W::AnchorTop : W::AnchorBottom));
    const int x = m_settings->clampX(m_settings->rightMargin(), available.width(), size.width());
    const int y = m_settings->clampY(m_settings->topMargin(), available.height(), size.height());
    m_layer->setMargins(QMargins(left ? x : 0, top ? y : 0, left ? 0 : x, top ? 0 : y));
}
void LayerShellWindowController::applySize(int w, int h) {
    m_settings->setWidth(w);
    m_settings->setHeight(h);
}
void LayerShellWindowController::setPaletteOpen(bool open) {
    m_paletteOpen = open;
    updateGeometry();
}
void LayerShellWindowController::resizeBy(int dx, int dy) {
    applySize(m_settings->width() + dx, m_settings->height() + dy);
}
void LayerShellWindowController::beginDrag(int gx, int gy) {
    if (!m_layer)
        return;
    m_startX = gx;
    m_startY = gy;
    const auto margins = m_layer->margins();
    m_startTop = qMax(margins.top(), margins.bottom());
    m_startRight = qMax(margins.left(), margins.right());
}
void LayerShellWindowController::dragTo(int gx, int gy) {
    if (!m_window || !m_window->screen())
        return;
    const QSize available = m_window->screen()->availableGeometry().size();
    const auto corner = m_settings->corner();
    const int x = m_startRight + (gx - m_startX) * (corner.endsWith(QStringLiteral("left")) ? 1 : -1);
    const int y = m_startTop + (gy - m_startY) * (corner.startsWith(QStringLiteral("top")) ? 1 : -1);
    m_settings->setRightMargin(m_settings->clampX(x, available.width(), m_window->width()));
    m_settings->setTopMargin(m_settings->clampY(y, available.height(), m_window->height()));
}
