#pragma once
#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>
namespace LayerShellQt {
class Window;
}
class SettingsManager;
class LayerShellWindowController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
  public:
    explicit LayerShellWindowController(SettingsManager *settings, QObject *parent = nullptr);
    bool initialize(QQuickWindow *window);
    Q_INVOKABLE void resizeBy(int dx, int dy);
    Q_INVOKABLE void beginDrag(int globalX, int globalY);
    Q_INVOKABLE void dragTo(int globalX, int globalY);
    Q_INVOKABLE void applySize(int width, int height);
    Q_INVOKABLE void setPaletteOpen(bool open);

  private:
    QPointer<QQuickWindow> m_window;
    LayerShellQt::Window *m_layer = nullptr;
    SettingsManager *m_settings;
    int m_startX = 0, m_startY = 0, m_startTop = 24, m_startRight = 24;
    bool m_paletteOpen = false;
    void updateGeometry();
    void watchScreen(QScreen *screen);
};
