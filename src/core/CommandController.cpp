#include "CommandController.hpp"
#include "DocumentStore.hpp"
#include "SettingsManager.hpp"
#include <QRegularExpression>

ParsedCommand ParsedCommand::parse(QString input) {
    input = input.trimmed();
    if (input.startsWith(QLatin1Char(':')))
        input.remove(0, 1);
    const auto words = input.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    ParsedCommand result;
    auto invalid = [&result](const QString &message) {
        result.error = message;
        return result;
    };
    if (words.isEmpty())
        return invalid(QStringLiteral("Enter a command. Try :help."));
    result.verb = words.first();
    result.arguments = words.mid(1);
    const auto &a = result.arguments;
    if (QStringList{QStringLiteral("help"), QStringLiteral("collapse"), QStringLiteral("expand"),
                    QStringLiteral("hide"), QStringLiteral("quit")}
            .contains(result.verb)) {
        if (!a.isEmpty())
            return invalid(QStringLiteral("This command takes no arguments."));
    } else if (result.verb == QStringLiteral("edit")) {
        if (a.size() != 1 || !QStringList{QStringLiteral("goals"), QStringLiteral("notes")}.contains(a[0]))
            return invalid(QStringLiteral("Use :edit goals or :edit notes."));
    } else if (result.verb == QStringLiteral("goal")) {
        if (a.size() < 2)
            return invalid(
                QStringLiteral("Use :goal add <text>, :goal toggle <index>, or :goal delete <index>."));
        if (a[0] == QStringLiteral("add")) {
            static const QRegularExpression prefix(QStringLiteral("^\\s*goal\\s+add\\s+"));
            input.remove(prefix);
            if (input.contains(QLatin1Char('\n')) || input.contains(QLatin1Char('\r')))
                return invalid(QStringLiteral("Add one checklist item at a time."));
            result.arguments = {QStringLiteral("add"), input};
        } else {
            bool ok = false;
            const int index = a[1].toInt(&ok);
            if (a.size() != 2 || !ok || index < 1 ||
                (a[0] != QStringLiteral("toggle") && a[0] != QStringLiteral("delete")))
                return invalid(QStringLiteral("Use a positive task number, for example :goal toggle 1."));
        }
    } else if (result.verb == QStringLiteral("zoom")) {
        if (a.size() != 1 ||
            !QStringList{QStringLiteral("+"), QStringLiteral("-"), QStringLiteral("reset")}.contains(a[0]))
            return invalid(QStringLiteral("Use :zoom +, :zoom -, or :zoom reset."));
    } else if (result.verb == QStringLiteral("theme")) {
        if (a.size() != 1 ||
            !QStringList{QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("system")}.contains(
                a[0]))
            return invalid(QStringLiteral("Use :theme light, :theme dark, or :theme system."));
    } else if (result.verb == QStringLiteral("position")) {
        if (a.size() != 1 || !QStringList{QStringLiteral("top-left"), QStringLiteral("top-right"),
                                          QStringLiteral("bottom-left"), QStringLiteral("bottom-right")}
                                  .contains(a[0]))
            return invalid(QStringLiteral("Choose top-left, top-right, bottom-left, or bottom-right."));
    } else if (result.verb == QStringLiteral("resize")) {
        bool wok = false, hok = false;
        const int width = a.value(0).toInt(&wok), height = a.value(1).toInt(&hok);
        if (a.size() != 2 || !wok || !hok || width < 280 || width > 720 || height < 260 || height > 900)
            return invalid(QStringLiteral("Use :resize <width> <height>; width 280–720, height 260–900."));
    } else
        return invalid(QStringLiteral("Unknown command. Try :help."));
    return result;
}

QString Checklist::change(const QString &source, int index, bool remove, QString *error) {
    static const QRegularExpression task(
        QStringLiteral("^(?: {0,3}[-+*]| {0,3}\\d+[.)])[ \\t]+\\[([ xX])\\](?=[ \\t\\r]|$)"));
    static const QRegularExpression fence(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    QChar fenceCharacter;
    int fenceLength = 0, count = 0;
    for (qsizetype start = 0; start < source.size();) {
        const qsizetype end = source.indexOf(QLatin1Char('\n'), start);
        const qsizetype next = end < 0 ? source.size() : end + 1;
        const QString line = source.mid(start, (end < 0 ? source.size() : end) - start);
        const auto f = fence.match(line);
        if (f.hasMatch()) {
            const auto marker = f.captured(1);
            if (fenceLength == 0) {
                fenceCharacter = marker[0];
                fenceLength = marker.size();
            } else if (marker[0] == fenceCharacter && marker.size() >= fenceLength &&
                       f.captured(2).trimmed().isEmpty())
                fenceLength = 0;
        } else if (fenceLength == 0) {
            const auto m = task.match(line);
            if (m.hasMatch() && ++count == index) {
                QString result = source;
                if (remove)
                    result.remove(start, next - start);
                else
                    result[start + m.capturedStart(1)] =
                        m.captured(1) == QStringLiteral(" ") ? QLatin1Char('x') : QLatin1Char(' ');
                return result;
            }
        }
        start = next;
    }
    *error = QStringLiteral("Task %1 does not exist (%2 checklist items). Indexes start at 1 in goals.md.")
                 .arg(index)
                 .arg(count);
    return source;
}
QString Checklist::append(const QString &source, const QString &text, QString *error) {
    // Do not append a supposed task inside an unfinished fenced code block.
    const QRegularExpression fence(QStringLiteral("^ {0,3}(`{3,}|~{3,})(.*)$"));
    QChar character;
    int length = 0;
    for (const auto &line : source.split(QLatin1Char('\n'))) {
        const auto match = fence.match(line);
        if (!match.hasMatch())
            continue;
        const auto marker = match.captured(1);
        if (!length) {
            character = marker[0];
            length = marker.size();
        } else if (marker[0] == character && marker.size() >= length && match.captured(2).trimmed().isEmpty())
            length = 0;
    }
    if (length) {
        if (error)
            *error = QStringLiteral("Close the unfinished fenced code block in goals before adding a task.");
        return source;
    }
    const QString newline =
        source.contains(QStringLiteral("\r\n")) ? QStringLiteral("\r\n") : QStringLiteral("\n");
    return source + ((!source.isEmpty() && !source.endsWith(QLatin1Char('\n'))) ? newline : QString()) +
           QStringLiteral("- [ ] ") + text + newline;
}
CommandController::CommandController(DocumentStore *d, SettingsManager *s, QObject *p)
    : QObject(p), m_documents(d), m_settings(s) {}
QString CommandController::help() {
    return QStringLiteral(
        ":edit goals | :edit notes\n:goal add <text> | :goal toggle <index> | :goal delete <index>\n:zoom + "
        "| :zoom - | :zoom reset\n:theme light | dark | system\n:collapse | :expand\n:position top-left | "
        "top-right | bottom-left | bottom-right\n:resize <width> <height>\n:hide | :quit\n\nTasks are "
        "numbered from 1 in source order.\nTab completes; ↑/↓ recalls history.\nCtrl+Enter or Escape leaves "
        "editing and flushes saves.");
}
QString CommandController::execute(const QString &input) {
    const auto c = ParsedCommand::parse(input);
    if (!input.trimmed().isEmpty() && (m_history.isEmpty() || m_history.last() != input)) {
        m_history.append(input);
        if (m_history.size() > 100)
            m_history.removeFirst();
    }
    if (!c.error.isEmpty())
        return c.error;
    const auto &a = c.arguments;
    if (c.verb == QStringLiteral("help"))
        return help();
    if (c.verb == QStringLiteral("edit")) {
        m_settings->setCompact(false);
        emit editRequested(a[0]);
    }
    if (c.verb == QStringLiteral("goal")) {
        QString error;
        const QString next = a[0] == QStringLiteral("add")
                                 ? Checklist::append(m_documents->goals(), a[1], &error)
                                 : Checklist::change(m_documents->goals(), a[1].toInt(),
                                                     a[0] == QStringLiteral("delete"), &error);
        if (!error.isEmpty())
            return error;
        m_documents->setDocument(QStringLiteral("goals"), next);
    }
    if (c.verb == QStringLiteral("zoom"))
        m_settings->setZoom(a[0] == QStringLiteral("reset")
                                ? 16
                                : m_settings->zoom() + (a[0] == QStringLiteral("+") ? 1 : -1));
    if (c.verb == QStringLiteral("theme"))
        m_settings->setTheme(a[0]);
    if (c.verb == QStringLiteral("collapse"))
        m_settings->setCompact(true);
    if (c.verb == QStringLiteral("expand"))
        m_settings->setCompact(false);
    if (c.verb == QStringLiteral("position"))
        m_settings->setCorner(a[0]);
    if (c.verb == QStringLiteral("resize")) {
        m_settings->setWidth(a[0].toInt());
        m_settings->setHeight(a[1].toInt());
    }
    if (c.verb == QStringLiteral("hide")) {
        m_documents->flush();
        emit hideRequested();
    }
    if (c.verb == QStringLiteral("quit")) {
        m_documents->flush();
        emit quitRequested();
    }
    return QString();
}
QString CommandController::complete(const QString &input) const {
    const QStringList options = {QStringLiteral(":help"),
                                 QStringLiteral(":edit goals"),
                                 QStringLiteral(":edit notes"),
                                 QStringLiteral(":goal add "),
                                 QStringLiteral(":goal toggle "),
                                 QStringLiteral(":goal delete "),
                                 QStringLiteral(":zoom +"),
                                 QStringLiteral(":zoom -"),
                                 QStringLiteral(":zoom reset"),
                                 QStringLiteral(":theme light"),
                                 QStringLiteral(":theme dark"),
                                 QStringLiteral(":theme system"),
                                 QStringLiteral(":collapse"),
                                 QStringLiteral(":expand"),
                                 QStringLiteral(":position top-left"),
                                 QStringLiteral(":position top-right"),
                                 QStringLiteral(":position bottom-left"),
                                 QStringLiteral(":position bottom-right"),
                                 QStringLiteral(":resize "),
                                 QStringLiteral(":hide"),
                                 QStringLiteral(":quit")};
    const QString prefix = input.startsWith(QLatin1Char(':')) ? input : QLatin1Char(':') + input;
    for (const auto &option : options)
        if (option.startsWith(prefix))
            return option;
    return input;
}
QString CommandController::history(int offset) const {
    return offset >= 0 && offset < m_history.size() ? m_history[m_history.size() - 1 - offset] : QString();
}
