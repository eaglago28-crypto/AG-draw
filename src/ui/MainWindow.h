#pragma once

#include <QMainWindow>

namespace agdraw::ui {

class CanvasView;
class ToolBox;
class PropertiesBar;
class LayersPanel;
class ColorPalette;

// Fenêtre principale : assemble tous les panneaux de l'écran de travail.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    CanvasView *m_canvas;
    ToolBox *m_toolBox;
    PropertiesBar *m_propertiesBar;
    LayersPanel *m_layersPanel;
    ColorPalette *m_colorPalette;
};

} // namespace agdraw::ui
