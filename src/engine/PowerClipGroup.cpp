#include "PowerClipGroup.h"

#include "EllipseShape.h"
#include "RectShape.h"

#include <QPainter>
#include <QPainterPath>

namespace agdraw::engine {

PowerClipGroup::PowerClipGroup(std::unique_ptr<Shape> container, std::vector<std::unique_ptr<Shape>> contents)
    : m_container(std::move(container)), m_contents(std::move(contents)) {}

void PowerClipGroup::translate(const QPointF &delta) {
    m_container->translate(delta);
    for (auto &content : m_contents) {
        content->translate(delta);
    }
}

void PowerClipGroup::paint(QPainter &painter) const {
    QPainterPath clipPath;
    if (dynamic_cast<EllipseShape *>(m_container.get())) {
        clipPath.addEllipse(m_container->bounds());
    } else {
        clipPath.addRect(m_container->bounds());
    }

    painter.save();
    painter.setClipPath(clipPath, Qt::IntersectClip);
    for (const auto &content : m_contents) {
        content->paint(painter);
    }
    painter.restore();

    painter.setPen(QPen(m_container->strokeColor, m_container->strokeWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(clipPath);
}

std::unique_ptr<Shape> PowerClipGroup::releaseContainer() {
    return std::move(m_container);
}

std::vector<std::unique_ptr<Shape>> PowerClipGroup::releaseContents() {
    return std::move(m_contents);
}

} // namespace agdraw::engine
