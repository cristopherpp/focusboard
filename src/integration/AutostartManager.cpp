#include "AutostartManager.hpp"
#include "InstallPaths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace {
QString execArgument(QString path) {
    // Desktop Entry string escaping followed by Exec argument escaping.
    path.replace(QLatin1Char('\\'), QStringLiteral("\\\\\\\\"));
    path.replace(QLatin1Char('"'), QStringLiteral("\\\\\""));
    path.replace(QLatin1Char('$'), QStringLiteral("\\\\$"));
    path.replace(QLatin1Char('`'), QStringLiteral("\\\\`"));
    path.replace(QLatin1Char('%'), QStringLiteral("%%"));
    return QLatin1Char('"') + path + QLatin1Char('"');
}
} // namespace
AutostartManager::AutostartManager(QObject *parent)
    : AutostartManager(QCoreApplication::applicationFilePath(), QStringLiteral(FOCUSBOARD_INSTALL_EXECUTABLE),
                       parent) {}
AutostartManager::AutostartManager(QString executable, QString installedExecutable, QObject *parent)
    : QObject(parent), m_executable(std::move(executable)),
      m_installedExecutable(std::move(installedExecutable)) {}
QString AutostartManager::entryPath() const {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
        .filePath(QStringLiteral("autostart/focusboard.desktop"));
}
bool AutostartManager::fail(const QString &message) {
    if (m_error != message) {
        m_error = message;
        emit errorChanged();
    }
    return false;
}
bool AutostartManager::isEnabled() const {
    if (!QFile::exists(entryPath()))
        return false;
    QSettings entry(entryPath(), QSettings::IniFormat);
    entry.beginGroup(QStringLiteral("Desktop Entry"));
    return !entry.value(QStringLiteral("Hidden"), false).toBool() &&
           entry.value(QStringLiteral("X-GNOME-Autostart-enabled"), true).toBool();
}
QString AutostartManager::desktopEntry(const QString &executable) {
    QString tryExec = executable;
    tryExec.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    return QStringLiteral("[Desktop "
                          "Entry]\nType=Application\nName=Focusboard\nExec=%1\nTryExec=%2\nIcon="
                          "focusboard\nTerminal=false\nStartupNotify=false\nX-KDE-autostart-after=panel\n")
        .arg(execArgument(executable), tryExec);
}
bool AutostartManager::save(const QString &text) {
    const QString path = entryPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return fail(tr("Cannot create autostart directory: %1").arg(path));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return fail(tr("Cannot write autostart entry: %1").arg(file.errorString()));
    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size() || !file.commit())
        return fail(tr("Cannot save autostart entry: %1").arg(file.errorString()));
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }
    return true;
}
bool AutostartManager::setEnabled(bool enabled) {
    if (enabled) {
        if (m_executable.contains(QLatin1Char('\n')) || m_executable.contains(QLatin1Char('\r')))
            return fail(tr("Cannot create an autostart entry for an executable path containing a newline."));
        return save(desktopEntry(m_executable));
    }
    if (QFile::exists(entryPath()) && !QFile::remove(entryPath()))
        return fail(tr("Cannot remove autostart entry: %1").arg(entryPath()));
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }
    return true;
}
bool AutostartManager::migrateExistingEntry() {
    // Never enable login startup implicitly, or redirect an installed entry back into a developer build.
    if (QDir::cleanPath(m_executable) != QDir::cleanPath(m_installedExecutable) ||
        !QFile::exists(entryPath()))
        return true;
    QFile file(entryPath());
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Cannot read autostart entry: %1").arg(file.errorString()));
    const QString original = QString::fromUtf8(file.readAll());
    if (file.error() != QFileDevice::NoError)
        return fail(tr("Cannot read autostart entry: %1").arg(file.errorString()));
    file.close();
    QStringList lines = original.split(QLatin1Char('\n'));
    bool inEntry = false, foundExec = false, foundTryExec = false;
    QString tryExec = m_executable;
    tryExec.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    for (QString &line : lines) {
        const auto trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('[')))
            inEntry = trimmed == QStringLiteral("[Desktop Entry]");
        if (!inEntry)
            continue;
        if (line.startsWith(QStringLiteral("Exec="))) {
            line = QStringLiteral("Exec=") + execArgument(m_executable);
            foundExec = true;
        }
        if (line.startsWith(QStringLiteral("TryExec="))) {
            line = QStringLiteral("TryExec=") + tryExec;
            foundTryExec = true;
        }
    }
    if (!foundExec)
        return fail(
            tr("The existing autostart entry has no Exec field; enable Launch at login again to repair it."));
    if (!foundTryExec) {
        const qsizetype section = lines.indexOf(QStringLiteral("[Desktop Entry]"));
        if (section >= 0)
            lines.insert(section + 1, QStringLiteral("TryExec=") + tryExec);
    }
    const QString updated = lines.join(QLatin1Char('\n'));
    // Preserve Hidden, KDE/GNOME disable flags, conditions, and other user customizations.
    return updated == original || save(updated);
}
