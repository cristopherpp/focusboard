#include "AutostartManager.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace {
const QString runKey = QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QString valueName = QStringLiteral("Focusboard");
} // namespace
AutostartManager::AutostartManager(QObject *parent)
    : AutostartManager(QCoreApplication::applicationFilePath(), runKey, parent) {}
// On Windows the second parameter is an injectable registry key for isolated tests.
AutostartManager::AutostartManager(QString executable, QString registryKey, QObject *parent)
    : QObject(parent), m_executable(std::move(executable)), m_installedExecutable(std::move(registryKey)) {}
AutostartManager::AutostartManager(QString executable, QString location, QSettings::Format format,
                                   QObject *parent)
    : AutostartManager(std::move(executable), std::move(location), parent) {
    m_registryFormat = format;
}
bool AutostartManager::isInstalled() const { return true; }
bool AutostartManager::hasConfiguration() const {
    QSettings registry(m_installedExecutable, m_registryFormat);
    return registry.contains(valueName);
}
bool AutostartManager::isEnabled() const {
    QSettings registry(m_installedExecutable, m_registryFormat);
    return !registry.value(valueName).toString().isEmpty();
}
bool AutostartManager::migrateExistingEntry() { return true; }
bool AutostartManager::fail(const QString &message) {
    m_error = message;
    emit errorChanged();
    return false;
}
bool AutostartManager::setEnabled(bool enabled) {
    if (m_executable.contains(QLatin1Char('"')) || m_executable.contains(QLatin1Char('\n')) ||
        m_executable.contains(QLatin1Char('\r')))
        return fail(tr("Invalid executable path for autostart."));
    QSettings registry(m_installedExecutable, m_registryFormat);
    if (enabled)
        registry.setValue(valueName,
                          QLatin1Char('"') + QDir::toNativeSeparators(m_executable) + QLatin1Char('"'));
    else
        registry.remove(valueName);
    registry.sync();
    if (registry.status() != QSettings::NoError)
        return fail(tr("Cannot update the per-user Windows autostart entry."));
    m_error.clear();
    emit errorChanged();
    emit stateChanged();
    return true;
}
