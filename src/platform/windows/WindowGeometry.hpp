#pragma once
#include <QRect>
#include <QString>
#include <algorithm>

namespace WindowsGeometry {
// QScreen/QWindow coordinates are logical. This also works for monitors left of
// or above the primary monitor; no physical-coordinate origin scaling occurs.
inline QRect constrain(QRect requested, const QRect &available) {
    if (available.isEmpty())
        return QRect(available.topLeft(), QSize(0, 0));
    requested.setSize(QSize(std::clamp(requested.width(), 0, available.width()),
                            std::clamp(requested.height(), 0, available.height())));
    requested.moveLeft(
        std::clamp(requested.x(), available.left(), available.right() - requested.width() + 1));
    requested.moveTop(
        std::clamp(requested.y(), available.top(), available.bottom() - requested.height() + 1));
    return requested;
}
inline QPoint anchoredPosition(const QRect &area, const QSize &size, const QString &corner,
                               int horizontalMargin, int verticalMargin) {
    return QPoint(
        corner.endsWith(QStringLiteral("left")) ? area.left() + horizontalMargin
                                                : area.right() - size.width() - horizontalMargin + 1,
        corner.startsWith(QStringLiteral("top")) ? area.top() + verticalMargin
                                                 : area.bottom() - size.height() - verticalMargin + 1);
}
} // namespace WindowsGeometry
