#pragma once
#include <QObject>
#include <QStringList>
#ifdef Q_OS_WIN
#include <QSettings>
#endif
#include <QtQml/qqmlregistration.h>
class AutostartManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  public:
    explicit AutostartManager(QObject *parent = nullptr);
    // Linux: installed executable path. Windows: registration key. Production uses platform defaults.
    AutostartManager(QString executable, QString installedExecutable, QObject *parent = nullptr);
#ifdef Q_OS_WIN
    // Inject a temporary INI store to exercise registration without touching HKCU.
    AutostartManager(QString executable, QString registrationLocation, QSettings::Format format,
                     QObject *parent = nullptr);
#endif
    Q_INVOKABLE bool setEnabled(bool enabled);
    Q_INVOKABLE bool isEnabled() const;
    Q_INVOKABLE bool hasConfiguration() const;
    bool isInstalled() const;
    bool migrateExistingEntry();
#ifndef Q_OS_WIN
    static QString desktopEntry(const QString &executable);
#endif
    QString error() const { return m_error; }
  signals:
    void errorChanged();
    void stateChanged();

  private:
    QString m_error;
    QString m_executable;
    QString m_installedExecutable;
#ifdef Q_OS_WIN
    QSettings::Format m_registryFormat = QSettings::NativeFormat;
#else
    QString entryPath() const;
    QStringList legacyPaths() const;
    bool save(const QString &text);
    bool setEntryEnabled(const QString &path, bool enabled);
#endif
    bool fail(const QString &message);
};
