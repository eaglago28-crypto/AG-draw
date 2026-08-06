#include "TextToCurves.h"

#include "CompoundPathShape.h"
#include "TextShape.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainterPath>

namespace agdraw::engine {

std::vector<std::unique_ptr<Shape>> textToCurves(const TextShape &text) {
    std::vector<std::unique_ptr<Shape>> result;

    QFont font;
    font.setPointSizeF(text.fontPointSize);
    const QFontMetricsF metrics(font);

    const QStringList lines = text.text.split(QLatin1Char('\n'));
    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const QString &line = lines.at(lineIndex);
        qreal x = text.position.x();
        const qreal y = text.position.y() + lineIndex * metrics.lineSpacing();

        for (const QChar &ch : line) {
            const qreal advance = metrics.horizontalAdvance(ch);
            if (!ch.isSpace()) {
                QPainterPath glyphPath;
                glyphPath.addText(QPointF(x, y), font, QString(ch));
                if (!glyphPath.isEmpty()) {
                    std::vector<QVector<QPointF>> contours;
                    for (const QPolygonF &polygon : glyphPath.toSubpathPolygons()) {
                        contours.push_back(polygon);
                    }
                    if (!contours.empty()) {
                        auto shape = std::make_unique<CompoundPathShape>(std::move(contours));
                        shape->fillColor = text.fillColor;
                        shape->strokeColor = text.strokeColor;
                        shape->strokeWidth = 0.0;
                        result.push_back(std::move(shape));
                    }
                }
            }
            x += advance;
        }
    }

    return result;
}

} // namespace agdraw::engine
