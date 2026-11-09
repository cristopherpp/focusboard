#include "SettingsManager.hpp"
#include <QGuiApplication>
#include <QStyleHints>
#include <algorithm>
SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), m_settings(QStringLiteral("Focusboard"), QStringLiteral("Focusboard")) {
    if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        connect(app->styleHints(), &QStyleHints::colorSchemeChanged, this,
                &SettingsManager::systemDarkChanged);
}
bool SettingsManager::systemDark() const {
    auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    return app && app->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}
int SettingsManager::zoom() const { return m_settings.value(QStringLiteral("zoom"), 16).toInt(); }
void SettingsManager::setZoom(int v) {
    v = std::clamp(v, 10, 32);
    if (v == zoom())
        return;
    m_settings.setValue(QStringLiteral("zoom"), v);
    emit zoomChanged();
}
QString SettingsManager::theme() const {
    return m_settings.value(QStringLiteral("theme"), QStringLiteral("system")).toString();
}
void SettingsManager::setTheme(const QString &v) {
    if (!QStringList{QStringLiteral("system"), QStringLiteral("light"), QStringLiteral("dark")}.contains(v) ||
        v == theme())
        return;
    m_settings.setValue(QStringLiteral("theme"), v);
    emit themeChanged();
}
QString SettingsManager::corner() const {
    return m_settings.value(QStringLiteral("corner"), QStringLiteral("top-right")).toString();
}
void SettingsManager::setCorner(const QString &v) {
    if (!QStringList{QStringLiteral("top-left"), QStringLiteral("top-right"), QStringLiteral("bottom-left"),
                     QStringLiteral("bottom-right")}
             .contains(v))
        return;
    m_settings.setValue(QStringLiteral("corner"), v);
    m_settings.setValue(QStringLiteral("topMargin"), margin());
    m_settings.setValue(QStringLiteral("rightMargin"), margin());
    emit positionChanged();
}
bool SettingsManager::compact() const { return m_settings.value(QStringLiteral("compact"), false).toBool(); }
void SettingsManager::setCompact(bool v) {
    if (v == compact())
        return;
    m_settings.setValue(QStringLiteral("compact"), v);
    emit compactChanged();
}
int SettingsManager::width() const { return m_settings.value(QStringLiteral("width"), 360).toInt(); }
void SettingsManager::setWidth(int v) {
    v = std::clamp(v, 280, 720);
    if (v == width())
        return;
    m_settings.setValue(QStringLiteral("width"), v);
    emit sizeChanged();
}
int SettingsManager::height() const { return m_settings.value(QStringLiteral("height"), 440).toInt(); }
void SettingsManager::setHeight(int v) {
    v = std::clamp(v, 260, 900);
    if (v == height())
        return;
    m_settings.setValue(QStringLiteral("height"), v);
    emit sizeChanged();
}
int SettingsManager::margin() const {
    return std::clamp(m_settings.value(QStringLiteral("margin"), 24).toInt(), 0, 200);
}
void SettingsManager::setMargin(int v) {
    v = std::clamp(v, 0, 200);
    if (v == margin())
        return;
    m_settings.setValue(QStringLiteral("margin"), v);
    emit marginChanged();
}
int SettingsManager::topMargin() const {
    return m_settings.value(QStringLiteral("topMargin"), margin()).toInt();
}
void SettingsManager::setTopMargin(int v) {
    v = std::clamp(v, 0, 10000);
    if (v == topMargin())
        return;
    m_settings.setValue(QStringLiteral("topMargin"), v);
    emit positionChanged();
}
int SettingsManager::rightMargin() const {
    return m_settings.value(QStringLiteral("rightMargin"), margin()).toInt();
}
void SettingsManager::setRightMargin(int v) {
    v = std::clamp(v, 0, 10000);
    if (v == rightMargin())
        return;
    m_settings.setValue(QStringLiteral("rightMargin"), v);
    emit positionChanged();
}
bool SettingsManager::autostart() const {
    return m_settings.value(QStringLiteral("autostart"), false).toBool();
}
void SettingsManager::setAutostart(bool v) {
    if (v == autostart())
        return;
    m_settings.setValue(QStringLiteral("autostart"), v);
    emit autostartChanged();
}
int SettingsManager::clampX(int x, int sw, int ww) const { return std::clamp(x, 0, std::max(0, sw - ww)); }
int SettingsManager::clampY(int y, int sh, int wh) const { return std::clamp(y, 0, std::max(0, sh - wh)); }
