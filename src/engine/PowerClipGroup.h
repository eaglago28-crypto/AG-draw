#pragma once

#include "Shape.h"

#include <memory>
#include <vector>

namespace agdraw::engine {

// PowerClip (CorelDRAW) : place un ou plusieurs objets « contenu » à
// l'intérieur d'un objet « contenant » (rectangle ou ellipse), en masquant
// tout ce qui dépasse ses limites. bounds()/contains()/isResizable()
// délèguent au contenant, ce qui permet de redimensionner un PowerClip
// avec les poignées habituelles : seule la fenêtre de découpe bouge, le
// contenu garde sa position absolue (limitation connue, cohérente avec les
// autres effets qui ne recalculent pas leurs sous-éléments).
class PowerClipGroup : public Shape {
public:
    PowerClipGroup(std::unique_ptr<Shape> container, std::vector<std::unique_ptr<Shape>> contents);

    QRectF bounds() const override { return m_container->bounds(); }
    bool contains(const QPointF &point) const override { return m_container->contains(point); }
    void translate(const QPointF &delta) override;
    void setBounds(const QRectF &rect) override { m_container->setBounds(rect); }
    bool isResizable() const override { return m_container->isResizable(); }
    void paint(QPainter &painter) const override;

    Shape *container() const { return m_container.get(); }
    const std::vector<std::unique_ptr<Shape>> &contents() const { return m_contents; }

    // Démonte le groupe (utilisé par ApplyPowerClipCommand::undo() pour
    // restituer les formes d'origine au calque). Le groupe reste en vie,
    // vidé de son contenu, prêt à être repeuplé par setContainer()/
    // setContents() si la commande est rejouée (redo) : ApplyPowerClipCommand
    // conserve le même objet PowerClipGroup d'un cycle undo/redo à l'autre
    // pour que les pointeurs déjà détenus ailleurs (sélection courante)
    // restent valides.
    std::unique_ptr<Shape> releaseContainer();
    std::vector<std::unique_ptr<Shape>> releaseContents();
    void setContainer(std::unique_ptr<Shape> container) { m_container = std::move(container); }
    void setContents(std::vector<std::unique_ptr<Shape>> contents) { m_contents = std::move(contents); }

private:
    std::unique_ptr<Shape> m_container;
    std::vector<std::unique_ptr<Shape>> m_contents;
};

} // namespace agdraw::engine
