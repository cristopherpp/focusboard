#pragma once
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
class AutostartManager final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  public:
    explicit AutostartManager(QObject *parent = nullptr);
    // Dependency injection for migration tests; production uses the installed path and current executable.
    AutostartManager(QString executable, QString installedExecutable, QObject *parent = nullptr);
    Q_INVOKABLE bool setEnabled(bool enabled);
    Q_INVOKABLE bool isEnabled() const;
    Q_INVOKABLE bool hasConfiguration() const;
    bool isInstalled() const;
    bool migrateExistingEntry();
    static QString desktopEntry(const QString &executable);
    QString error() const { return m_error; }
  signals:
    void errorChanged();
    void stateChanged();

  private:
    QString m_error;
    QString m_executable;
    QString m_installedExecutable;
    QString entryPath() const;
    QStringList legacyPaths() const;
    bool save(const QString &text);
    bool setEntryEnabled(const QString &path, bool enabled);
    bool fail(const QString &message);
};
