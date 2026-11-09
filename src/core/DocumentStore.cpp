#include "DocumentStore.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringDecoder>

DocumentStore::DocumentStore(QObject *parent) : QObject(parent) {
    m_directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &DocumentStore::flush);
    load();
}
void DocumentStore::load() {
    auto read = [this](const QString &name, QString &value, bool &ok) {
        QFile file(QDir(m_directory).filePath(name));
        if (!file.exists())
            return;
        if (!QFileInfo(file).isFile()) {
            ok = false;
            setError(tr("Cannot read %1: expected a regular file").arg(name));
            return;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            ok = false;
            setError(tr("Cannot read %1: %2").arg(name, file.errorString()));
            return;
        }
        const QByteArray bytes = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            ok = false;
            setError(tr("Cannot read %1: %2").arg(name, file.errorString()));
            return;
        }
        QStringDecoder decoder(QStringDecoder::Utf8);
        const QString decoded = decoder.decode(bytes);
        if (decoder.hasError()) {
            ok = false;
            setError(tr("Cannot read %1: invalid UTF-8; original file was preserved").arg(name));
            return;
        }
        value = decoded;
    };
    read(QStringLiteral("goals.md"), m_goals, m_goalsLoaded);
    read(QStringLiteral("notes.md"), m_notes, m_notesLoaded);
}
void DocumentStore::setError(const QString &message) {
    if (m_error == message)
        return;
    m_error = message;
    emit errorChanged();
}
void DocumentStore::setDocument(const QString &name, const QString &text) {
    if (name == QStringLiteral("goals")) {
        if (m_goals == text)
            return;
        m_goals = text;
        m_goalsDirty = true;
        emit goalsChanged();
    } else if (name == QStringLiteral("notes")) {
        if (m_notes == text)
            return;
        m_notes = text;
        m_notesDirty = true;
        emit notesChanged();
    } else
        return;
    m_saveTimer.start();
}
bool DocumentStore::writeFile(const QString &name, const QString &text) {
    QDir dir;
    if (!dir.mkpath(m_directory)) {
        setError(tr("Cannot create data directory: %1").arg(m_directory));
        return false;
    }
    QSaveFile file(QDir(m_directory).filePath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Cannot save %1: %2").arg(name, file.errorString()));
        return false;
    }
    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        setError(tr("Cannot save %1: %2").arg(name, file.errorString()));
        return false;
    }
    return true;
}
void DocumentStore::flush() {
    m_saveTimer.stop();
    bool success = m_goalsLoaded && m_notesLoaded;
    if (m_goalsLoaded && m_goalsDirty) {
        const bool saved = writeFile(QStringLiteral("goals.md"), m_goals);
        success = saved && success;
        if (saved)
            m_goalsDirty = false;
    }
    if (m_notesLoaded && m_notesDirty) {
        const bool saved = writeFile(QStringLiteral("notes.md"), m_notes);
        success = saved && success;
        if (saved)
            m_notesDirty = false;
    }
    if (success)
        setError(QString());
}
QString DocumentStore::summary(const QString &name) const {
    const QString content = name == QStringLiteral("goals") ? m_goals : m_notes;
    const QStringList lines = content.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    return lines.isEmpty() ? tr("Nothing written yet") : lines.first().left(90);
}
