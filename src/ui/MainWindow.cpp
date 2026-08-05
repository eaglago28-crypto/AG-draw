#include "MainWindow.h"

#include "CanvasView.h"
#include "ToolBox.h"
#include "PropertiesBar.h"
#include "LayersPanel.h"
#include "ColorPalette.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

namespace agdraw::ui {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1400, 900);

    m_canvas = new CanvasView(this);
    setCentralWidget(m_canvas);

    setupFileMenu();

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
    m_layersPanel->setDocument(&m_canvas->document());

    connect(m_colorPalette, &ColorPalette::colorSelected, m_canvas, &CanvasView::setActiveColor);
    connect(m_canvas, &CanvasView::statusMessage, this, [this](const QString &text) {
        statusBar()->showMessage(text, 3000);
    });
    connect(m_canvas, &CanvasView::selectionChanged, m_propertiesBar, &PropertiesBar::setSelectedShape);
    connect(m_propertiesBar, &PropertiesBar::strokeWidthEdited, m_canvas, &CanvasView::setSelectionStrokeWidth);
    connect(m_canvas, &CanvasView::toolShortcutRequested, this, [this](Tool tool) {
        const auto actions = m_toolBox->actions();
        const int index = static_cast<int>(tool);
        if (index >= 0 && index < actions.size()) {
            actions[index]->trigger();
        }
    });

    updateWindowTitle();
    statusBar()->showMessage(tr("Prêt"));
}

void MainWindow::setupFileMenu() {
    auto *fileMenu = menuBar()->addMenu(tr("&Fichier"));

    QAction *newAction = fileMenu->addAction(tr("&Nouveau"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::doNew);

    QAction *openAction = fileMenu->addAction(tr("&Ouvrir…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::doOpen);

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(tr("&Enregistrer"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::doSave);

    QAction *saveAsAction = fileMenu->addAction(tr("Enregistrer &sous…"));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::doSaveAs);

    fileMenu->addSeparator();

    QAction *exportAction = fileMenu->addAction(tr("&Exporter en PNG…"));
    exportAction->setShortcut(QKeySequence(tr("Ctrl+E")));
    connect(exportAction, &QAction::triggered, this, &MainWindow::doExportPng);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(tr("&Quitter"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

bool MainWindow::confirmDiscardIfNeeded() {
    if (m_canvas->isEmpty()) {
        return true;
    }
    const auto answer = QMessageBox::question(
        this, tr("Document non enregistré"),
        tr("Le document actuel contient des éléments non enregistrés. Continuer et perdre ces modifications ?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Yes;
}

void MainWindow::updateWindowTitle() {
    const QString name = m_currentFilePath.isEmpty() ? tr("Sans titre") : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(tr("AG Draw — %1").arg(name));
}

void MainWindow::doNew() {
    if (!confirmDiscardIfNeeded()) {
        return;
    }
    m_canvas->newDocument();
    m_currentFilePath.clear();
    updateWindowTitle();
}

void MainWindow::doOpen() {
    if (!confirmDiscardIfNeeded()) {
        return;
    }
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Ouvrir un document AG Draw"), QString(), tr("Documents AG Draw (*.agd)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_canvas->loadFromFile(path, &error)) {
        QMessageBox::warning(this, tr("Échec de l'ouverture"), error);
        return;
    }
    m_currentFilePath = path;
    updateWindowTitle();
}

void MainWindow::doSave() {
    if (m_currentFilePath.isEmpty()) {
        doSaveAs();
        return;
    }
    QString error;
    if (!m_canvas->saveToFile(m_currentFilePath, &error)) {
        QMessageBox::warning(this, tr("Échec de l'enregistrement"), error);
    } else {
        statusBar()->showMessage(tr("Document enregistré"), 3000);
    }
}

void MainWindow::doSaveAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Enregistrer sous"), QString(), tr("Documents AG Draw (*.agd)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".agd"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".agd");
    }
    QString error;
    if (!m_canvas->saveToFile(path, &error)) {
        QMessageBox::warning(this, tr("Échec de l'enregistrement"), error);
        return;
    }
    m_currentFilePath = path;
    updateWindowTitle();
    statusBar()->showMessage(tr("Document enregistré"), 3000);
}

void MainWindow::doExportPng() {
    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter en PNG"), QString(), tr("Images PNG (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!m_canvas->exportToPng(path, &error)) {
        QMessageBox::warning(this, tr("Échec de l'export"), error);
    } else {
        statusBar()->showMessage(tr("Image exportée"), 3000);
    }
}

} // namespace agdraw::ui
