#include "MarkdownPreview.hpp"
#include <QAbstractTextDocumentLayout>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
namespace {
QString withPreviewSpacing(QString source) {
    source.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    source.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const auto lines = QStringView(source).split(QLatin1Char('\n'));
    static const QRegularExpression fencePattern(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    static const QRegularExpression listItem(QStringLiteral("^ {0,3}(?:[-+*]|[0-9]{1,9}[.)])(?:\\s|$)"));
    QString result;
    QString fence;
    qsizetype blanks = 0;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const auto line = lines.at(i);
        if (fence.isEmpty() && line.trimmed().isEmpty()) {
            // A final newline terminates the last line without creating another blank row.
            if (i + 1 < lines.size() || !line.isEmpty())
                ++blanks;
            continue;
        }
        if (blanks) {
            // Leave list separation and indentation to the Markdown block parser.
            if (line.startsWith(QLatin1Char(' ')) || line.startsWith(QLatin1Char('\t')) ||
                listItem.matchView(line).hasMatch()) {
                result += QString(blanks, QLatin1Char('\n'));
            } else {
                result += QStringLiteral("\n\u200b\n").repeated(blanks);
                result += QLatin1Char('\n');
            }
            blanks = 0;
        }
        result += line;
        const auto match = fencePattern.matchView(line);
        if (match.hasMatch()) {
            const auto marker = match.capturedView(1);
            const auto tail = match.capturedView(2);
            if (fence.isEmpty()) {
                if (marker.front() != QLatin1Char('`') || !tail.contains(QLatin1Char('`')))
                    fence = marker.toString();
            } else if (marker.startsWith(fence) && tail.trimmed().isEmpty()) {
                fence.clear();
            }
        } else if (fence.isEmpty() && !line.startsWith(QStringLiteral("    ")) &&
                   !line.startsWith(QLatin1Char('\t')) && !line.endsWith(QLatin1Char('\\'))) {
            // Two trailing spaces request a Markdown hard break in a paragraph.
            result += QStringLiteral("  ");
        }
        if (i + 1 < lines.size())
            result += QLatin1Char('\n');
    }
    result += QStringLiteral("\n\u200b\n").repeated(blanks);
    return result;
}
} // namespace

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
    m_document.setMarkdown(withPreviewSpacing(value),
                           QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub) |
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
