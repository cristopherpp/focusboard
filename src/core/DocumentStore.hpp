#pragma once
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class DocumentStore final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QString goals READ goals NOTIFY goalsChanged)
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool unsaved READ unsaved NOTIFY statusChanged)
    Q_PROPERTY(bool conflicted READ conflicted NOTIFY statusChanged)
  public:
    explicit DocumentStore(QObject *parent = nullptr);
    QString goals() const { return m_goals.text; }
    QString notes() const { return m_notes.text; }
    QString error() const { return m_error; }
    bool unsaved() const { return m_goals.dirty || m_notes.dirty; }
    bool conflicted() const { return m_goals.conflict || m_notes.conflict; }
    Q_INVOKABLE void setDocument(const QString &name, const QString &text);
    Q_INVOKABLE bool flush();
    Q_INVOKABLE QString summary(const QString &name) const;
    QString dataDirectory() const { return m_directory; }
    void scanExternalChanges();
  signals:
    void goalsChanged();
    void notesChanged();
    void errorChanged();
    void statusChanged();

  private:
    struct Document {
        QString fileName;
        QString text;
        QByteArray savedBytes;
        QByteArray conflictBytes;
        QString conflictCopy;
        bool existed = false;
        bool loaded = true;
        bool dirty = false;
        bool conflict = false;
    };
    struct DiskState {
        QByteArray bytes;
        QString error;
        bool exists = false;
        bool ok = true;
    };
    QString m_directory, m_error;
    Document m_goals{QStringLiteral("goals.md")}, m_notes{QStringLiteral("notes.md")};
    QFileSystemWatcher m_watcher;
    QTimer m_saveTimer, m_scanTimer;
    int m_retryCount = 0;
    DiskState readDisk(const Document &document) const;
    void load(Document &document);
    bool writeFile(Document &document);
    bool saveConflictCopy(Document &document);
    void scan(Document &document);
    void refreshWatchPaths();
    void setError(const QString &message);
    void setDirty(Document &document, bool value);
};
