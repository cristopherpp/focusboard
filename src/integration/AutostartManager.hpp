#pragma once
#include <QObject>
class AutostartManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  public:
    explicit AutostartManager(QObject *parent = nullptr);
    // Dependency injection for migration tests; production uses the installed path and current executable.
    AutostartManager(QString executable, QString installedExecutable, QObject *parent = nullptr);
    Q_INVOKABLE bool setEnabled(bool enabled);
    Q_INVOKABLE bool isEnabled() const;
    bool migrateExistingEntry();
    static QString desktopEntry(const QString &executable);
    QString error() const { return m_error; }
  signals:
    void errorChanged();

  private:
    QString m_error;
    QString m_executable;
    QString m_installedExecutable;
    QString entryPath() const;
    bool save(const QString &text);
    bool fail(const QString &message);
};
