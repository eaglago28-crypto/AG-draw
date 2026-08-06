#include "ShapeRecognizer.h"

#include "EllipseShape.h"
#include "RectShape.h"

#include <QLineF>
#include <algorithm>
#include <cmath>

namespace agdraw::engine {

namespace {
constexpr int kMinPoints = 8;
constexpr qreal kMinSize = 4.0;
constexpr qreal kMaxClosingGapRatio = 0.3; // par rapport à la diagonale du cadre englobant.
constexpr qreal kMaxRmsError = 0.25;

QRectF boundingBoxOf(const QVector<QPointF> &points) {
    qreal minX = points.first().x();
    qreal maxX = minX;
    qreal minY = points.first().y();
    qreal maxY = minY;
    for (const QPointF &p : points) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

qreal ellipseRmsError(const QVector<QPointF> &points, const QPointF &center, qreal rx, qreal ry) {
    qreal sumSq = 0.0;
    for (const QPointF &p : points) {
        const qreal nx = (p.x() - center.x()) / rx;
        const qreal ny = (p.y() - center.y()) / ry;
        const qreal r = std::hypot(nx, ny);
        sumSq += (r - 1.0) * (r - 1.0);
    }
    return std::sqrt(sumSq / points.size());
}

qreal rectangleRmsError(const QVector<QPointF> &points, const QRectF &bbox) {
    const qreal normalizer = std::min(bbox.width(), bbox.height());
    if (normalizer <= 0.0) {
        return 1.0;
    }
    qreal sumSq = 0.0;
    for (const QPointF &p : points) {
        const qreal dLeft = std::abs(p.x() - bbox.left());
        const qreal dRight = std::abs(p.x() - bbox.right());
        const qreal dTop = std::abs(p.y() - bbox.top());
        const qreal dBottom = std::abs(p.y() - bbox.bottom());
        const qreal d = std::min({dLeft, dRight, dTop, dBottom}) / normalizer;
        sumSq += d * d;
    }
    return std::sqrt(sumSq / points.size());
}

} // namespace

ShapeRecognitionResult recognizeShape(const QVector<QPointF> &points) {
    ShapeRecognitionResult result;
    if (points.size() < kMinPoints) {
        return result;
    }

    const QRectF bbox = boundingBoxOf(points);
    if (bbox.width() < kMinSize || bbox.height() < kMinSize) {
        return result;
    }

    const qreal diagonal = std::hypot(bbox.width(), bbox.height());
    const qreal closingGap = QLineF(points.first(), points.last()).length();
    if (closingGap > diagonal * kMaxClosingGapRatio) {
        return result; // le tracé ne forme pas une boucle fermée.
    }

    const QPointF center = bbox.center();
    const qreal rx = bbox.width() / 2.0;
    const qreal ry = bbox.height() / 2.0;

    const qreal ellipseError = ellipseRmsError(points, center, rx, ry);
    const qreal rectError = rectangleRmsError(points, bbox);

    if (ellipseError <= kMaxRmsError && ellipseError <= rectError) {
        result.kind = RecognizedShapeKind::Ellipse;
        result.shape = std::make_unique<EllipseShape>(bbox);
    } else if (rectError <= kMaxRmsError) {
        result.kind = RecognizedShapeKind::Rectangle;
        result.shape = std::make_unique<RectShape>(bbox);
    }

    return result;
}

} // namespace agdraw::engine
