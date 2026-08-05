#pragma once

#include <QMainWindow>

namespace agdraw::ui {

class CanvasView;
class ToolBox;
class PropertiesBar;
class LayersPanel;
class PagesPanel;
class ColorPalette;

// Fenêtre principale : assemble tous les panneaux de l'écran de travail.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupFileMenu();
    bool confirmDiscardIfNeeded();
    void updateWindowTitle();
    void doNew();
    void doOpen();
    void doSave();
    void doSaveAs();
    void doExportPng();

    CanvasView *m_canvas;
    ToolBox *m_toolBox;
    PropertiesBar *m_propertiesBar;
    LayersPanel *m_layersPanel;
    PagesPanel *m_pagesPanel;
    ColorPalette *m_colorPalette;
    QString m_currentFilePath;
};

} // namespace agdraw::ui
