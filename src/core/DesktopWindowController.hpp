#pragma once
#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QScreen>
#include <QSize>
#include <QtQml/qqmlregistration.h>
namespace LayerShellQt {
class Window;
}
class SettingsManager;
class DesktopWindowController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
  public:
    explicit DesktopWindowController(SettingsManager *settings, QObject *parent = nullptr);
    bool initialize(QQuickWindow *window, QScreen *screen = nullptr);
    Q_INVOKABLE void resizeBy(int dx, int dy);
    Q_INVOKABLE void beginDrag(int globalX, int globalY);
    Q_INVOKABLE void dragTo(int globalX, int globalY);
    Q_INVOKABLE void applySize(int width, int height);
    Q_INVOKABLE void setPaletteOpen(bool open);
    static QSize clampSurfaceSize(const QSize &requested, const QSize &available);

  private:
    QPointer<QQuickWindow> m_window;
#ifdef Q_OS_WIN
    QPointer<QScreen> m_screen;
    QRect m_geometry;
    QString m_geometryKey;
    void saveGeometry();
    void restack();
    bool eventFilter(QObject *object, QEvent *event) override;
#else
    LayerShellQt::Window *m_layer = nullptr;
#endif
    SettingsManager *m_settings;
    int m_startX = 0, m_startY = 0, m_startTop = 24, m_startRight = 24;
    bool m_paletteOpen = false;
    void updateGeometry();
    void watchScreen(QScreen *screen);
};
