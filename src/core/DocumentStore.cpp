#include "DocumentStore.hpp"
#include <QDateTime>
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
    m_scanTimer.setSingleShot(true);
    m_scanTimer.setInterval(80);
    connect(&m_scanTimer, &QTimer::timeout, this, &DocumentStore::scanExternalChanges);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_scanTimer, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_scanTimer, qOverload<>(&QTimer::start));
    load(m_goals);
    load(m_notes);
    refreshWatchPaths();
}
DocumentStore::DiskState DocumentStore::readDisk(const Document &document) const {
    DiskState state;
    const QString path = QDir(m_directory).filePath(document.fileName);
    QFileInfo info(path);
    if (!info.exists() && !info.isSymLink())
        return state;
    state.exists = true;
    if (!info.isFile()) {
        state.ok = false;
        state.error = tr("Cannot read %1: expected a regular file").arg(document.fileName);
        return state;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        state.ok = false;
        state.error = tr("Cannot read %1: %2").arg(document.fileName, file.errorString());
        return state;
    }
    state.bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        state.ok = false;
        state.error = tr("Cannot read %1: %2").arg(document.fileName, file.errorString());
        return state;
    }
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString decoded = decoder.decode(state.bytes);
    Q_UNUSED(decoded);
    if (decoder.hasError()) {
        state.ok = false;
        state.error = tr("Cannot read %1: invalid UTF-8; original file was preserved").arg(document.fileName);
    }
    return state;
}
void DocumentStore::load(Document &document) {
    const DiskState disk = readDisk(document);
    if (!disk.ok) {
        document.loaded = false;
        setError(disk.error);
        return;
    }
    document.existed = disk.exists;
    document.savedBytes = disk.bytes;
    document.text = QString::fromUtf8(disk.bytes);
}
void DocumentStore::setError(const QString &message) {
    if (m_error == message)
        return;
    m_error = message;
    emit errorChanged();
}
void DocumentStore::setDirty(Document &document, bool value) {
    if (document.dirty == value)
        return;
    document.dirty = value;
    emit statusChanged();
}
void DocumentStore::setDocument(const QString &name, const QString &text) {
    Document *document = name == QStringLiteral("goals")   ? &m_goals
                         : name == QStringLiteral("notes") ? &m_notes
                                                           : nullptr;
    if (!document || document->text == text)
        return;
    document->text = text;
    setDirty(*document, true);
    if (document == &m_goals)
        emit goalsChanged();
    else
        emit notesChanged();
    m_retryCount = 0;
    m_saveTimer.start(400);
}
bool DocumentStore::writeFile(Document &document) {
    if (!QDir().mkpath(m_directory)) {
        setError(tr("Cannot create data directory: %1").arg(m_directory));
        return false;
    }
    QSaveFile file(QDir(m_directory).filePath(document.fileName));
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Cannot save %1: %2").arg(document.fileName, file.errorString()));
        return false;
    }
    const QByteArray bytes = document.text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        setError(tr("Cannot save %1: %2").arg(document.fileName, file.errorString()));
        return false;
    }
    const DiskState disk = readDisk(document);
    if (!disk.ok || disk.exists != document.existed || disk.bytes != document.savedBytes) {
        file.cancelWriting();
        scan(document);
        return false;
    }
    if (!file.commit()) {
        setError(tr("Cannot save %1: %2").arg(document.fileName, file.errorString()));
        return false;
    }
    return true;
}
bool DocumentStore::saveConflictCopy(Document &document) {
    const QByteArray bytes = document.text.toUtf8();
    if (!document.conflictCopy.isEmpty() && document.conflictBytes == bytes &&
        QFile::exists(document.conflictCopy))
        return true;
    if (document.conflictCopy.isEmpty()) {
        const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzz"));
        document.conflictCopy = QDir(m_directory)
                                    .filePath(document.fileName + QStringLiteral(".local-conflict-") + stamp +
                                              QStringLiteral(".md"));
    }
    QSaveFile copy(document.conflictCopy);
    if (!copy.open(QIODevice::WriteOnly))
        return false;
    if (copy.write(bytes) != bytes.size() || !copy.commit())
        return false;
    document.conflictBytes = bytes;
    return true;
}
void DocumentStore::scan(Document &document) {
    const DiskState disk = readDisk(document);
    if (!disk.ok) {
        if (document.dirty) {
            if (!document.conflict) {
                document.conflict = true;
                emit statusChanged();
            }
            const bool copied = saveConflictCopy(document);
            setError(tr("%1 changed or cannot be read. Local edits %2; resolve manually.")
                         .arg(document.fileName, copied ? tr("saved to %1").arg(document.conflictCopy)
                                                        : tr("remain only in memory")));
        } else {
            document.loaded = false;
            setError(disk.error);
        }
        return;
    }
    if (document.existed == disk.exists && document.savedBytes == disk.bytes) {
        if (!document.loaded && !document.dirty) {
            document.loaded = true;
            if (m_goals.loaded && m_notes.loaded && !conflicted())
                setError(QString());
        }
        return;
    }
    if (document.dirty) {
        if (!document.conflict) {
            document.conflict = true;
            emit statusChanged();
        }
        const bool copied = saveConflictCopy(document);
        setError(tr("%1 changed outside Focusboard. Local edits %2; resolve manually.")
                     .arg(document.fileName, copied ? tr("saved to %1").arg(document.conflictCopy)
                                                    : tr("remain only in memory")));
        return;
    }
    document.existed = disk.exists;
    document.savedBytes = disk.bytes;
    document.text = QString::fromUtf8(disk.bytes);
    document.loaded = true;
    if (&document == &m_goals)
        emit goalsChanged();
    else
        emit notesChanged();
    if (!m_goals.conflict && !m_notes.conflict && m_goals.loaded && m_notes.loaded)
        setError(QString());
}
void DocumentStore::scanExternalChanges() {
    scan(m_goals);
    scan(m_notes);
    refreshWatchPaths();
}
void DocumentStore::refreshWatchPaths() {
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && QFileInfo(ancestor).absolutePath() != ancestor)
        ancestor = QFileInfo(ancestor).absolutePath();
    for (const QString &path :
         {ancestor, QFileInfo(m_directory).absolutePath(), m_directory,
          QDir(m_directory).filePath(m_goals.fileName), QDir(m_directory).filePath(m_notes.fileName)}) {
        QFileInfo info(path);
        if (info.exists() && !m_watcher.directories().contains(path) && !m_watcher.files().contains(path))
            m_watcher.addPath(path);
    }
}
bool DocumentStore::flush() {
    m_saveTimer.stop();
    scanExternalChanges(); // Detect a replacement even when the watcher event has not arrived yet.
    bool success = true;
    for (Document *document : {&m_goals, &m_notes}) {
        if (!document->dirty)
            continue;
        if (!document->loaded || document->conflict) {
            success = false;
            continue;
        }
        if (!writeFile(*document)) {
            success = false;
            continue;
        }
        document->existed = true;
        document->savedBytes = document->text.toUtf8();
        setDirty(*document, false);
    }
    refreshWatchPaths();
    if (success) {
        m_retryCount = 0;
        if (m_goals.loaded && m_notes.loaded && !conflicted())
            setError(QString());
    } else if (unsaved() && !conflicted() && m_goals.loaded && m_notes.loaded && m_retryCount < 3) {
        m_saveTimer.start(1000 << m_retryCount++); // 1, 2, then 4 seconds; a new edit resets the budget.
    }
    return success;
}
QString DocumentStore::summary(const QString &name) const {
    const QString content = name == QStringLiteral("goals") ? m_goals.text : m_notes.text;
    const QStringList lines = content.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    return lines.isEmpty() ? tr("Nothing written yet") : lines.first().left(90);
}
