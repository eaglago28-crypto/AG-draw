#pragma once

#include <QRectF>
#include <QString>

namespace agdraw::engine {

// Une page est un rectangle nommé dans l'espace de dessin continu du
// document (comme dans CorelDRAW : les pages ne sont pas des conteneurs
// de calques séparés, juste des repères visuels et des cibles
// d'export/impression). Les formes restent partagées entre toutes les
// pages, positionnées dans le même système de coordonnées.
class Page {
public:
    Page(QString name, QRectF rect) : m_name(std::move(name)), m_rect(rect) {}

    const QString &name() const { return m_name; }
    void setName(QString name) { m_name = std::move(name); }

    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rect) { m_rect = rect; }

private:
    QString m_name;
    QRectF m_rect;
};

} // namespace agdraw::engine
