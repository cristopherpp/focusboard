#pragma once
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
  public:
    explicit DocumentStore(QObject *parent = nullptr);
    QString goals() const { return m_goals; }
    QString notes() const { return m_notes; }
    QString error() const { return m_error; }
    Q_INVOKABLE void setDocument(const QString &name, const QString &text);
    Q_INVOKABLE void flush();
    Q_INVOKABLE QString summary(const QString &name) const;
    QString dataDirectory() const { return m_directory; }
  signals:
    void goalsChanged();
    void notesChanged();
    void errorChanged();

  private:
    QString m_directory, m_goals, m_notes, m_error;
    bool m_goalsLoaded = true, m_notesLoaded = true;
    bool m_goalsDirty = false, m_notesDirty = false;
    QTimer m_saveTimer;
    void load();
    bool writeFile(const QString &name, const QString &text);
    void setError(const QString &message);
};
