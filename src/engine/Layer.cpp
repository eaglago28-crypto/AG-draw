#include "Layer.h"

#include <algorithm>

namespace agdraw::engine {

Shape *Layer::addShape(std::unique_ptr<Shape> shape) {
    m_shapes.push_back(std::move(shape));
    return m_shapes.back().get();
}

Shape *Layer::insertShape(size_t index, std::unique_ptr<Shape> shape) {
    index = std::min(index, m_shapes.size());
    auto it = m_shapes.insert(m_shapes.begin() + static_cast<std::ptrdiff_t>(index), std::move(shape));
    return it->get();
}

size_t Layer::indexOf(Shape *shape) const {
    for (size_t i = 0; i < m_shapes.size(); ++i) {
        if (m_shapes[i].get() == shape) {
            return i;
        }
    }
    return m_shapes.size();
}

void Layer::removeShape(Shape *shape) {
    m_shapes.erase(
        std::remove_if(m_shapes.begin(), m_shapes.end(),
                        [shape](const std::unique_ptr<Shape> &candidate) { return candidate.get() == shape; }),
        m_shapes.end());
}

std::unique_ptr<Shape> Layer::takeShape(Shape *shape) {
    auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
                            [shape](const std::unique_ptr<Shape> &candidate) { return candidate.get() == shape; });
    if (it == m_shapes.end()) {
        return nullptr;
    }
    std::unique_ptr<Shape> taken = std::move(*it);
    m_shapes.erase(it);
    return taken;
}

void Layer::moveShape(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= m_shapes.size() || toIndex >= m_shapes.size() || fromIndex == toIndex) {
        return;
    }
    auto from = m_shapes.begin() + static_cast<std::ptrdiff_t>(fromIndex);
    auto to = m_shapes.begin() + static_cast<std::ptrdiff_t>(toIndex);
    if (fromIndex < toIndex) {
        std::rotate(from, from + 1, to + 1);
    } else {
        std::rotate(to, from, from + 1);
    }
}

void Layer::paint(QPainter &painter) const {
    if (!m_visible) {
        return;
    }
    for (const auto &shape : m_shapes) {
        shape->paint(painter);
    }
}

} // namespace agdraw::engine
