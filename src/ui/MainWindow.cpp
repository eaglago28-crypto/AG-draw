#include "MainWindow.h"

#include "CanvasView.h"
#include "ToolBox.h"
#include "PropertiesBar.h"
#include "LayersPanel.h"
#include "ColorPalette.h"

#include <QStatusBar>

namespace agdraw::ui {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("AG Draw"));
    resize(1400, 900);

    m_canvas = new CanvasView(this);
    setCentralWidget(m_canvas);

    m_toolBox = new ToolBox(this);
    addToolBar(Qt::LeftToolBarArea, m_toolBox);

    m_propertiesBar = new PropertiesBar(this);
    addToolBar(Qt::TopToolBarArea, m_propertiesBar);
    connect(m_toolBox, &ToolBox::toolSelected, m_propertiesBar, &PropertiesBar::setActiveTool);
    connect(m_toolBox, &ToolBox::toolSelected, m_canvas, &CanvasView::setActiveTool);

    m_colorPalette = new ColorPalette(this);
    addToolBar(Qt::RightToolBarArea, m_colorPalette);

    m_layersPanel = new LayersPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_layersPanel);

    connect(m_colorPalette, &ColorPalette::colorSelected, this, [this](const QColor &color) {
        statusBar()->showMessage(tr("Couleur sélectionnée : %1").arg(color.name()), 3000);
    });
    connect(m_canvas, &CanvasView::statusMessage, this, [this](const QString &text) {
        statusBar()->showMessage(text, 3000);
    });

    statusBar()->showMessage(tr("Prêt"));
}

} // namespace agdraw::ui
