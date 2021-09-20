#include "FadcDataViewer.h"
#include "RolStruct.h"
#include "WfRootGraph.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <iostream>
#include <sstream>

using namespace fdec;
using namespace std;

FadcDataViewer::FadcDataViewer(QWidget *parent) : QWidget(parent)
{
    InitGui();

    // decoding
    file_reader = new EvioFileReader();
    event_parser = new EventParser();
    fadc_decoder = new Fadc250Decoder();

    event_parser -> RegisterRawDecoder(static_cast<int>(Bank_TagID::FADC), fadc_decoder);

    // init timing
    current_event_timing.resize(16);
}

FadcDataViewer::~FadcDataViewer()
{
}

void FadcDataViewer::InitGui()
{
    // gui
    pCanvas = new QMainCanvas(this);
    pCtrlPanel = new QWidget(this);
    pCtrlPanel -> setFixedWidth(400);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout -> addWidget(pCanvas);
    layout -> addWidget(pCtrlPanel);

    AddMenuBar();

    // setup functionalities
    QVBoxLayout *right_side_layout = new QVBoxLayout(pCtrlPanel);
    QGridLayout *ctrl_layout = new QGridLayout();
    right_side_layout -> addLayout(ctrl_layout);
 
    // 1) open file
    QPushButton *b_open_file = new QPushButton("&Open File: ", pCtrlPanel);
    file_indicator = new QLineEdit(pCtrlPanel);
    file_indicator -> setText("data/SoLIDCer516.dat.0");
    ctrl_layout -> addWidget(b_open_file, 0, 0);
    ctrl_layout -> addWidget(file_indicator, 0, 1);

    // 2) next event
    QPushButton *btn = new QPushButton("&Next Event", pCtrlPanel);
    QPushButton *btn_p = new QPushButton("&Previous Event", pCtrlPanel);
    ctrl_layout -> addWidget(btn_p, 1, 0);
    ctrl_layout -> addWidget(btn, 1, 1);

    // 3) timing informaiton
    timing_indicator = new QTableWidget(16, 2, this);
    //timing_indicator -> setFixedHeight(500);
    // evenly distribute cell width and height
    QHeaderView *header_view = timing_indicator -> horizontalHeader();
    header_view -> setSectionResizeMode(QHeaderView::Stretch);
    timing_indicator -> verticalHeader() -> setSectionResizeMode(QHeaderView::Stretch);
    // set first collumn default text
    for(int r = 0;r<16;r++) {
        for(int c = 0;c<2;c++) {
            QTableWidgetItem *pCell = timing_indicator -> item(r, c);
            if(!pCell) {
                pCell = new QTableWidgetItem;
                timing_indicator -> setItem(r, c, pCell);
            }
            if(c == 0)
                pCell -> setText(Form("Ch %2d Signal Peak Timing [ns]:", r));
            else 
                pCell -> setText("0");
        }
    }

    right_side_layout -> addWidget(timing_indicator);

    connect(btn, SIGNAL(clicked()), this, SLOT(Draw()));
    connect(btn_p, SIGNAL(clicked()), this, SLOT(DrawPreviousEvent()));
    connect(b_open_file, SIGNAL(clicked()), this, SLOT(OpenFileDialog()));
}

void FadcDataViewer::AddMenuBar()
{
    pMenuBar = new QMenuBar();

    // file menu
    pMenu = new QMenu("File");
    pMenuBar -> addMenu(pMenu);
    pMenu -> addAction("Save");
    pMenu -> addAction("Exit");

    // view menu
    pMenuBar -> addMenu(new QMenu("View"));
    // Edit menu
    pMenuBar -> addMenu(new QMenu("Edit"));
    // Format menu
    pMenuBar -> addMenu(new QMenu("Format"));
    // Help Menu
    pMenuBar -> addMenu(new QMenu("Help"));

    this->layout()->setMenuBar(pMenuBar);
}

void FadcDataViewer::SetFile(const char* path)
{
    file_reader -> CloseFile();
    fFile = path;

    file_reader -> SetFile(path);
    file_reader -> OpenFile();
}

std::vector<TMultiGraph*> FadcDataViewer::GetEvent()
{
    vector<TMultiGraph*> res;

    if(fFile.length() == 0) {
        cout<<"Error: evio file not set yet."<<endl;
        return res;
    }

    if(file_reader -> ReadNoCopy(&pBuf, &fBufLen) != S_SUCCESS) {
        cout<<"Warning: Finished reading from evio file."<<endl;
        return res;
    }

    event_parser -> ParseEvent(pBuf, fBufLen);
    const Fadc250Event &event = fadc_decoder -> GetDecodedEvent();

    int nch = 0;
    // analyze channel data
    for(auto &i: event.channels)
    {
        // get waveform
        Fadc250Data data = i;
        waveform_ana.Analyze(data);
        WfRootGraph g = get_waveform_graph(waveform_ana, data.raw);
        g.mg->SetTitle(Form("FADC channel %d", nch));
        res.push_back(g.mg);

        // get timing (for each peak) for current event
        current_event_timing[nch].clear();
        for(auto &i: g.result.peaks) {
            current_event_timing[nch].push_back(i.time);
        }

        nch++;
    }

    return res;
}

void FadcDataViewer::DrawAllChannels(const std::vector<TMultiGraph*> & mg)
{
    pCanvas -> DrawCanvas(mg, 4, 4);
}

void FadcDataViewer::Draw()
{
    std::vector<TMultiGraph*> mgs = GetEvent();
    DrawAllChannels(mgs);

    // update timing for peaks in each channel
    for(int i=0;i<16;i++) {
        std::stringstream stream;
        for(auto &t: current_event_timing[i])
            stream << std::fixed<<std::setprecision(1)<<t<<" | ";
        std::string ss = stream.str();
        timing_indicator -> item(i, 1) -> setText(ss.c_str());
    }

    // save one history event
    history_event = current_event;
    current_event = mgs;

    pCanvas -> Refresh();
}

void FadcDataViewer::DrawPreviousEvent()
{
    if(history_event.size() <= 0) {
        cout<<"Info: No history event saved..."<<endl;
        return;
    }

    DrawAllChannels(history_event);

    pCanvas -> Refresh();
}

void FadcDataViewer::OpenFileDialog()
{
    QString file_name = QFileDialog::getOpenFileName(
            this,
            "Open Document",
            QDir::currentPath(),
            "All files (*.*) ;; evio files (*.evio);; dat files (*.dat)");
    fFile = file_name.toStdString();
    file_indicator -> setText(file_name);

    SetFile(fFile.c_str());
}
