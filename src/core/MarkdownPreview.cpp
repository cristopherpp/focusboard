#include "MarkdownPreview.hpp"
#include <QAbstractTextDocumentLayout>
#include <QImage>
#include <QPainter>
QVariant LocalMarkdownDocument::loadResource(int, const QUrl &) {
    // Never delegate to Qt's default loader, including for file: or data: URLs.
    QImage empty(1, 1, QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    return empty;
}
MarkdownPreview::MarkdownPreview(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    m_document.setDocumentMargin(0);
    connect(this, &QQuickItem::widthChanged, this, &MarkdownPreview::relayout);
}
void MarkdownPreview::setSource(const QString &value) {
    if (value == m_source)
        return;
    m_source = value;
    m_document.setMarkdown(value, QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub) |
                                      QTextDocument::MarkdownNoHTML);
    relayout();
    emit sourceChanged();
}
void MarkdownPreview::setFontSize(int value) {
    if (value == m_fontSize)
        return;
    m_fontSize = value;
    relayout();
    emit fontSizeChanged();
}
void MarkdownPreview::setForeground(const QColor &value) {
    if (value == m_foreground)
        return;
    m_foreground = value;
    update();
    emit foregroundChanged();
}
void MarkdownPreview::relayout() {
    QFont font;
    font.setPixelSize(m_fontSize);
    m_document.setDefaultFont(font);
    m_document.setTextWidth(qMax(1.0, width()));
    setImplicitHeight(m_document.size().height());
    update();
}
void MarkdownPreview::paint(QPainter *painter) {
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, m_foreground);
    context.palette.setColor(QPalette::WindowText, m_foreground);
    context.palette.setColor(QPalette::Link, QColor(QStringLiteral("#849ef1")));
    m_document.documentLayout()->draw(painter, context);
}
