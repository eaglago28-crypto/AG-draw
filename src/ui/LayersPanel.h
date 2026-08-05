#pragma once

#include <QDockWidget>

class QListWidget;

namespace agdraw::ui {

// Panneau des calques à droite : liste, ajout, suppression, visibilité.
class LayersPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit LayersPanel(QWidget *parent = nullptr);

private:
    void addLayer(const QString &name);

    QListWidget *m_list;
    int m_layerCount = 0;
};

} // namespace agdraw::ui
