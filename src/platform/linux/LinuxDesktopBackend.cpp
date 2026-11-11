#include "DesktopWindowController.hpp"
#include "SettingsManager.hpp"
#include <LayerShellQt/Window>
#include <QScreen>
DesktopWindowController::DesktopWindowController(SettingsManager *s, QObject *p)
    : QObject(p), m_settings(s) {}
bool DesktopWindowController::initialize(QQuickWindow *window, QScreen *screen) {
    if (m_window)
        return m_window == window;
    m_window = window;
    if (screen)
        window->setScreen(screen);
    m_layer = LayerShellQt::Window::get(window);
    if (!m_layer)
        return false;
    // LayerShellQt 6.6+ binds the output through setScreen(). Set both the
    // QWindow and layer-shell target before the first show/map.
    if (screen)
        m_layer->setScreen(screen);
    m_layer->setScope(QStringLiteral("focusboard"));
    m_layer->setLayer(LayerShellQt::Window::LayerBottom);
    m_layer->setExclusiveZone(0);
    m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    m_layer->setActivateOnShow(false);
    m_layer->setCloseOnDismissed(false);
    connect(m_settings, &SettingsManager::positionChanged, this, &DesktopWindowController::updateGeometry);
    connect(m_settings, &SettingsManager::sizeChanged, this, &DesktopWindowController::updateGeometry);
    connect(m_settings, &SettingsManager::compactChanged, this, &DesktopWindowController::updateGeometry);
    connect(window, &QWindow::screenChanged, this, [this](QScreen *s) {
        watchScreen(s);
        updateGeometry();
    });
    watchScreen(screen ? screen : window->screen());
    updateGeometry();
    return true;
}
void DesktopWindowController::watchScreen(QScreen *screen) {
    if (!screen)
        return;
    connect(screen, &QScreen::availableGeometryChanged, this, &DesktopWindowController::updateGeometry,
            Qt::UniqueConnection);
    connect(screen, &QScreen::geometryChanged, this, &DesktopWindowController::updateGeometry,
            Qt::UniqueConnection);
    connect(screen, &QScreen::logicalDotsPerInchChanged, this, &DesktopWindowController::updateGeometry,
            Qt::UniqueConnection);
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, &DesktopWindowController::updateGeometry,
            Qt::UniqueConnection);
}
void DesktopWindowController::updateGeometry() {
    if (!m_window || !m_layer || !m_window->screen())
        return;
    const QSize available = m_window->screen()->availableGeometry().size();
    const QSize size = clampSurfaceSize(
        QSize(m_settings->width(), m_settings->compact() && !m_paletteOpen ? 84 : m_settings->height()),
        available);
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
QSize DesktopWindowController::clampSurfaceSize(const QSize &requested, const QSize &available) {
    return QSize(qMax(0, qMin(requested.width(), available.width())),
                 qMax(0, qMin(requested.height(), available.height())));
}
void DesktopWindowController::applySize(int w, int h) {
    m_settings->setWidth(w);
    m_settings->setHeight(h);
}
void DesktopWindowController::setPaletteOpen(bool open) {
    m_paletteOpen = open;
    updateGeometry();
}
void DesktopWindowController::resizeBy(int dx, int dy) {
    applySize(m_settings->width() + dx, m_settings->height() + dy);
}
void DesktopWindowController::beginDrag(int gx, int gy) {
    if (!m_layer)
        return;
    m_startX = gx;
    m_startY = gy;
    const auto margins = m_layer->margins();
    m_startTop = qMax(margins.top(), margins.bottom());
    m_startRight = qMax(margins.left(), margins.right());
}
void DesktopWindowController::dragTo(int gx, int gy) {
    if (!m_window || !m_window->screen())
        return;
    const QSize available = m_window->screen()->availableGeometry().size();
    const auto corner = m_settings->corner();
    const int x = m_startRight + (gx - m_startX) * (corner.endsWith(QStringLiteral("left")) ? 1 : -1);
    const int y = m_startTop + (gy - m_startY) * (corner.startsWith(QStringLiteral("top")) ? 1 : -1);
    m_settings->setRightMargin(m_settings->clampX(x, available.width(), m_window->width()));
    m_settings->setTopMargin(m_settings->clampY(y, available.height(), m_window->height()));
}
