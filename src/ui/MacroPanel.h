#pragma once

#include <QDockWidget>

class QListWidget;
class QToolButton;

namespace agdraw::engine {
class Document;
struct Macro;
}

namespace agdraw::ui {

// Panneau des macros à droite : démarre/arrête l'enregistrement d'une
// macro (voir CanvasView::startMacroRecording), liste les macros du
// document et permet de les rejouer sur la sélection courante ou de les
// supprimer.
class MacroPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit MacroPanel(QWidget *parent = nullptr);

    void setDocument(agdraw::engine::Document *document);
    void refresh();

signals:
    void recordingStartRequested();
    void recordingStopRequested(const QString &name);
    void macroPlayRequested(const agdraw::engine::Macro &macro);
    void documentChanged();

private:
    agdraw::engine::Document *m_document = nullptr;
    QListWidget *m_list;
    QToolButton *m_recordButton;
    bool m_recording = false;
};

} // namespace agdraw::ui
