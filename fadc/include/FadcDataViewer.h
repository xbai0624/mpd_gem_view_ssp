#ifndef FADC_DATA_VIEWER_H
#define FADC_DATA_VIEWER_H

#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QLineEdit>
#include <QTableWidget>
#include <vector>
#include <string>
#include <TMultiGraph.h>

#include "QMainCanvas.h"
#include "EvioFileReader.h"
#include "EventParser.h"
#include "Fadc250Decoder.h"
#include "WfAnalyzer.h"

class FadcDataViewer : public QWidget
{
    Q_OBJECT
public:
    FadcDataViewer(QWidget *parent);
    ~FadcDataViewer();

    void InitGui();
    void AddMenuBar();

    void DrawAllChannels(const std::vector<TMultiGraph*>& mg);
    void SetFile(const char* path);

    std::vector<TMultiGraph*> GetEvent();

public slots:
    void Draw();
    void DrawPreviousEvent();
    void OpenFileDialog();

private:
    QMainCanvas *pCanvas;
    QWidget *pCtrlPanel;
    QMenuBar *pMenuBar;
    QMenu *pMenu;
    QLineEdit* file_indicator;
    QTableWidget *timing_indicator;

    // decoding
    EvioFileReader *file_reader;
    EventParser *event_parser;
    fdec::Fadc250Decoder *fadc_decoder;
    fdec::Analyzer waveform_ana;

    // read event buffer from evio file
    const uint32_t *pBuf;
    uint32_t fBufLen;

    std::string fFile = "";

    // save 1 events for history
    std::vector<TMultiGraph*> history_event;
    std::vector<TMultiGraph*> current_event;
    // timing
    std::vector<std::vector<double>> current_event_timing;
};

#endif
