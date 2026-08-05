#pragma once

#include "Shape.h"

#include <QString>
#include <memory>
#include <vector>

class QPainter;

namespace agdraw::engine {

class Layer {
public:
    explicit Layer(QString name) : m_name(std::move(name)) {}

    const QString &name() const { return m_name; }
    void setName(QString name) { m_name = std::move(name); }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }

    Shape *addShape(std::unique_ptr<Shape> shape);
    void removeShape(Shape *shape);
    std::unique_ptr<Shape> takeShape(Shape *shape);

    const std::vector<std::unique_ptr<Shape>> &shapes() const { return m_shapes; }

    void paint(QPainter &painter) const;

private:
    QString m_name;
    bool m_visible = true;
    bool m_locked = false;
    std::vector<std::unique_ptr<Shape>> m_shapes;
};

} // namespace agdraw::engine
