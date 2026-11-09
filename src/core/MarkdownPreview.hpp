#pragma once
#include <QQuickPaintedItem>
#include <QTextDocument>
#include <QtQml/qqmlregistration.h>

class LocalMarkdownDocument final : public QTextDocument {
  public:
    using QTextDocument::QTextDocument;

  protected:
    QVariant loadResource(int, const QUrl &) override;
};
class MarkdownPreview : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY foregroundChanged)
  public:
    explicit MarkdownPreview(QQuickItem *parent = nullptr);
    QString source() const { return m_source; }
    void setSource(const QString &source);
    int fontSize() const { return m_fontSize; }
    void setFontSize(int value);
    QColor foreground() const { return m_foreground; }
    void setForeground(const QColor &value);
    void paint(QPainter *painter) override;
  signals:
    void sourceChanged();
    void fontSizeChanged();
    void foregroundChanged();

  private:
    LocalMarkdownDocument m_document;
    QString m_source;
    int m_fontSize = 16;
    QColor m_foreground = Qt::black;
    void relayout();
};
