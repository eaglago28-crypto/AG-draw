#include "TextShape.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>

namespace agdraw::engine {

namespace {
QFont makeFont(qreal pointSize) {
    QFont font;
    font.setPointSizeF(pointSize);
    return font;
}
} // namespace

QRectF TextShape::bounds() const {
    const QStringList lines = text.split(QLatin1Char('\n'));
    QFontMetricsF metrics(makeFont(fontPointSize));

    QRectF result;
    for (int i = 0; i < lines.size(); ++i) {
        QRectF lineRect = metrics.boundingRect(lines.at(i));
        lineRect.translate(position.x(), position.y() + i * metrics.lineSpacing());
        result = (i == 0) ? lineRect : result.united(lineRect);
    }
    return result;
}

bool TextShape::contains(const QPointF &point) const {
    return bounds().contains(point);
}

void TextShape::paint(QPainter &painter) const {
    painter.setFont(makeFont(fontPointSize));
    painter.setPen(fillColor);

    const QFontMetricsF metrics(painter.font());
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        painter.drawText(QPointF(position.x(), position.y() + i * metrics.lineSpacing()), lines.at(i));
    }
}

} // namespace agdraw::engine
