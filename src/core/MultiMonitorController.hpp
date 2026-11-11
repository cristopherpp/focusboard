#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QUrl>

class AutostartManager;
class CommandController;
class DocumentStore;
class DesktopWindowController;
class QQuickWindow;
class QScreen;
class QQmlEngine;
class SettingsManager;

class MultiMonitorController final : public QObject {
    Q_OBJECT
  public:
    MultiMonitorController(QQmlEngine *engine, DocumentStore *documents, SettingsManager *settings,
                           CommandController *commands, AutostartManager *autostart, bool offerAutostart,
                           QObject *parent = nullptr);
    ~MultiMonitorController() override;

    bool start();
    QList<QQuickWindow *> windows() const;
    QQuickWindow *primaryWindow() const;
    int surfaceCount() const;
    bool isVisible() const;
    void showAll(bool activate = false);
    void hideAll();

    struct ScreenDelta {
        QStringList added;
        QStringList removed;
    };
    static QStringList uniqueScreenNames(QStringList names);
    static ScreenDelta screenDelta(const QStringList &current, const QStringList &connected);
    static QUrl mainQmlUrl();

  private:
    struct Surface;
    QQmlEngine *m_engine;
    DocumentStore *m_documents;
    SettingsManager *m_settings;
    CommandController *m_commands;
    AutostartManager *m_autostart;
    bool m_offerAutostart;
    QHash<QString, Surface *> m_surfaces;
    bool m_started = false;
    bool m_visible = true;

    static QString screenKey(QScreen *screen);
    void synchronizeScreens();
    bool createSurface(QScreen *screen);
    void destroySurface(const QString &key);
};
