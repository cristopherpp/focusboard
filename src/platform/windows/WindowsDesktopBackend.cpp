#include "DesktopWindowController.hpp"
#include "SettingsManager.hpp"
#include "WindowGeometry.hpp"
#include <QEvent>
#include <QSettings>
#include <QTimer>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

DesktopWindowController::DesktopWindowController(SettingsManager *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

bool DesktopWindowController::initialize(QQuickWindow *window, QScreen *screen) {
    if (m_window)
        return m_window == window;
    if (!window || !screen)
        return false;
    m_window = window;
    m_screen = screen;
    window->setScreen(screen);
    window->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    // Qt owns geometry and per-monitor DPI conversion. Never multiply global
    // logical desktop coordinates by a single monitor's devicePixelRatio.
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (!style && GetLastError())
        return false;
    SetLastError(0);
    if (!SetWindowLongPtrW(hwnd, GWL_EXSTYLE, (style | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW) &&
        GetLastError())
        return false;
    if (!SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED))
        return false;
    m_geometryKey =
        QStringLiteral("windows/screens/") +
        QString::fromLatin1((screen->name() + QLatin1Char('|') + screen->serialNumber()).toUtf8().toHex());
    QSettings preferences(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral("Focusboard"),
                          QStringLiteral("Focusboard"));
    m_geometry = preferences.value(m_geometryKey).toRect();
    if (!m_geometry.isValid()) {
        const QRect area = screen->availableGeometry();
        const QSize size(m_settings->width(), m_settings->height());
        m_geometry =
            QRect(WindowsGeometry::anchoredPosition(area, size, m_settings->corner(),
                                                    m_settings->rightMargin(), m_settings->topMargin()),
                  size);
    }
    window->installEventFilter(this);
    connect(m_settings, &SettingsManager::compactChanged, this, &DesktopWindowController::updateGeometry);
    connect(m_settings, &SettingsManager::sizeChanged, this,
            [this] { applySize(m_settings->width(), m_settings->height()); });
    connect(m_settings, &SettingsManager::positionChanged, this, [this] {
        if (!m_screen)
            return;
        const QRect area = m_screen->availableGeometry();
        m_geometry.moveTopLeft(
            WindowsGeometry::anchoredPosition(area, m_geometry.size(), m_settings->corner(),
                                              m_settings->rightMargin(), m_settings->topMargin()));
        updateGeometry();
        saveGeometry();
    });
    watchScreen(screen);
    updateGeometry();
    return true;
}
void DesktopWindowController::restack() {
    if (m_window && m_window->isVisible())
        SetWindowPos(reinterpret_cast<HWND>(m_window->winId()), HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
bool DesktopWindowController::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::Show || event->type() == QEvent::WindowDeactivate)
        QTimer::singleShot(0, this, &DesktopWindowController::restack);
    if (event->type() == QEvent::DevicePixelRatioChange)
        QTimer::singleShot(0, this, &DesktopWindowController::updateGeometry);
    // Activation is allowed for editing; no focus stealing or periodic polling.
    return QObject::eventFilter(object, event);
}
void DesktopWindowController::watchScreen(QScreen *screen) {
    connect(screen, &QScreen::availableGeometryChanged, this, &DesktopWindowController::updateGeometry);
    connect(screen, &QScreen::geometryChanged, this, &DesktopWindowController::updateGeometry);
    connect(screen, &QScreen::logicalDotsPerInchChanged, this, &DesktopWindowController::updateGeometry);
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, &DesktopWindowController::updateGeometry);
}
QSize DesktopWindowController::clampSurfaceSize(const QSize &requested, const QSize &available) {
    return QSize(qMax(0, qMin(requested.width(), available.width())),
                 qMax(0, qMin(requested.height(), available.height())));
}
void DesktopWindowController::updateGeometry() {
    if (!m_window || !m_screen)
        return;
    const QRect area = m_screen->availableGeometry();
    if (area.isEmpty())
        return;
    m_geometry = WindowsGeometry::constrain(m_geometry, area);
    QRect displayed = m_geometry;
    if (m_settings->compact() && !m_paletteOpen)
        displayed.setHeight(qMin(84, area.height()));
    m_window->setGeometry(displayed);
}
void DesktopWindowController::saveGeometry() {
    if (m_geometryKey.isEmpty())
        return;
    QSettings preferences(QSettings::defaultFormat(), QSettings::UserScope, QStringLiteral("Focusboard"),
                          QStringLiteral("Focusboard"));
    preferences.setValue(m_geometryKey, m_geometry);
    preferences.sync();
}
void DesktopWindowController::applySize(int width, int height) {
    m_geometry.setSize(QSize(std::clamp(width, 280, 720), std::clamp(height, 260, 900)));
    updateGeometry();
    saveGeometry();
}
void DesktopWindowController::resizeBy(int dx, int dy) {
    applySize(m_geometry.width() + dx, m_geometry.height() + dy);
}
void DesktopWindowController::setPaletteOpen(bool open) {
    m_paletteOpen = open;
    updateGeometry();
}
void DesktopWindowController::beginDrag(int x, int y) {
    m_startX = x;
    m_startY = y;
    m_startRight = m_geometry.x();
    m_startTop = m_geometry.y();
}
void DesktopWindowController::dragTo(int x, int y) {
    m_geometry.moveTopLeft(QPoint(m_startRight + x - m_startX, m_startTop + y - m_startY));
    updateGeometry();
    saveGeometry();
}
