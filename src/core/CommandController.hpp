#pragma once
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
class DocumentStore;
class SettingsManager;

struct ParsedCommand {
    QString verb;
    QStringList arguments;
    QString error;
    static ParsedCommand parse(QString input);
};

// Indexes are one-based positions in the CURRENT source, never stored identities.
struct Checklist {
    static QString change(const QString &source, int index, bool remove, QString *error);
    static QString append(const QString &source, const QString &text, QString *error = nullptr);
};

class CommandController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
  public:
    CommandController(DocumentStore *documents, SettingsManager *settings, QObject *parent = nullptr);
    Q_INVOKABLE QString execute(const QString &input);
    Q_INVOKABLE QString complete(const QString &input) const;
    Q_INVOKABLE QString history(int offset) const;
    static QString help();
  signals:
    void editRequested(const QString &document);
    void hideRequested();
    void quitRequested();

  private:
    DocumentStore *m_documents;
    SettingsManager *m_settings;
    QStringList m_history;
};
