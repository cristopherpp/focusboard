#pragma once
#include <QObject>
#include <QSettings>
#include <QtQml/qqmlregistration.h>

class SettingsManager final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(bool systemDark READ systemDark NOTIFY systemDarkChanged)
    Q_PROPERTY(int zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool compact READ compact WRITE setCompact NOTIFY compactChanged)
    Q_PROPERTY(QString corner READ corner WRITE setCorner NOTIFY positionChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY sizeChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY sizeChanged)
    Q_PROPERTY(int margin READ margin WRITE setMargin NOTIFY marginChanged)
    Q_PROPERTY(int topMargin READ topMargin WRITE setTopMargin NOTIFY positionChanged)
    Q_PROPERTY(int rightMargin READ rightMargin WRITE setRightMargin NOTIFY positionChanged)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)
  public:
    explicit SettingsManager(QObject *parent = nullptr);
    bool systemDark() const;
    int zoom() const;
    void setZoom(int value);
    QString theme() const;
    void setTheme(const QString &value);
    bool compact() const;
    void setCompact(bool value);
    QString corner() const;
    void setCorner(const QString &value);
    int width() const;
    void setWidth(int value);
    int height() const;
    void setHeight(int value);
    int margin() const;
    void setMargin(int value);
    int topMargin() const;
    void setTopMargin(int value);
    int rightMargin() const;
    void setRightMargin(int value);
    bool autostart() const;
    void setAutostart(bool value);
    bool sync();
    Q_INVOKABLE int clampX(int x, int screenWidth, int widgetWidth) const;
    Q_INVOKABLE int clampY(int y, int screenHeight, int widgetHeight) const;
  signals:
    void zoomChanged();
    void themeChanged();
    void compactChanged();
    void sizeChanged();
    void marginChanged();
    void positionChanged();
    void autostartChanged();
    void systemDarkChanged();

  private:
    QSettings m_settings;
};
