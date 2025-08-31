#include "Viewer.h"
#include "InfoCenter.h"
#include "APVStripMapping.h"

#include <QGraphicsRectItem>
#include <QSpinBox>
#include <QLabel>
#include <QTabWidget>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QCoreApplication>

#include <TCanvas.h>
#include <TStyle.h>
#include <TFile.h>

#include <iostream>
#include <fstream>
#include <thread>

#define EYE_BALL_TRACKING

////////////////////////////////////////////////////////////////
// ctor

Viewer::Viewer(QWidget *parent) : QWidget(parent)
{
    if(!txt_parser.ReadConfigFile("config/gem.conf")) {
        std::cout<<__func__<<" failed loading config file."<<std::endl;
        exit(1);
    }

    InitGui();

    InitGEMAnalyzer();

    resize(sizeHint());
}


////////////////////////////////////////////////////////////////
// init gui

void Viewer::InitGui()
{
    SetNumberOfTabs();

    InitLayout();
    AddMenuBar();

    // need to init right first, b/c left requires a valid 'pRightCanvas' pointer
    InitRightView();
    InitLeftView();

    // detector components layout
    //InitComponentsSchematic(); 
    
    setWindowTitle("GEM Data Viewer");

    //connect(b, SIGNAL(clicked()), fCanvas1, SLOT(DrawCanvas()));
    //connect(fCanvas1, SIGNAL(ItemSelected()), componentsView, SLOT(ItemSelected()));
    //connect(fCanvas1, SIGNAL(ItemDeSelected()), componentsView, SLOT(ItemDeSelected()));
}

////////////////////////////////////////////////////////////////
// init layout

void Viewer::InitLayout()
{
    // main layout
    pMainLayout = new QVBoxLayout(this);   // menubar + content

    // the whole drawing area
    pDrawingArea = new QWidget(this);

    // content layout
    pDrawingLayout = new QHBoxLayout(pDrawingArea);    // left content + right content

    // left content
    pLeft = new QWidget(pDrawingArea);
    pLeftLayout = new QVBoxLayout(pLeft);       // left content

    // right content
    pRight = new QWidget(pDrawingArea);
    pRightLayout = new QVBoxLayout(pRight);      // right content

    pDrawingLayout -> addWidget(pLeft);
    pDrawingLayout -> addWidget(pRight);

    pMainLayout -> addWidget(pDrawingArea);
}

////////////////////////////////////////////////////////////////
// this function is used to draw some illustrating shapes for 
// demonstrating the detector APV setup, each setup is different
// whether use it or not depends on user's choice

void Viewer::InitComponentsSchematic()
{
    componentsView = new ComponentsSchematic(this);
    componentsView->resize(800, 400);
    componentsView -> setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored));
}

////////////////////////////////////////////////////////////////
// add a menu bar

void Viewer::AddMenuBar()
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
    // online analysis
    pOnlineAnalysis = new QMenu("Online Analysis");
    pOpenAnalysisInterface = new QAction("Interface", this);
    pOnlineAnalysis -> addAction(pOpenAnalysisInterface);
    pMenuBar -> addMenu(pOnlineAnalysis);
    // Help Menu
    pMenuBar -> addMenu(new QMenu("Help"));

    this->layout()->setMenuBar(pMenuBar);
}

////////////////////////////////////////////////////////////////
// init left drawing area

void Viewer::InitLeftView()
{
    InitLeftTab();
}

////////////////////////////////////////////////////////////////
// init right drawing area

void Viewer::InitRightView()
{
    // right content (show selected apv)
    pRightCanvas = new QMainCanvas(pRight);
    // right control interface
    pRightCtrlInterface = new QWidget(pRight);
    // fix the width in the right side layout
    pRight -> setFixedWidth(450);

    InitCtrlInterface();

    // setup a printing log window
    pLogBox = new QTextEdit(pRight);
    pLogBox -> setTextColor(QColor("black"));
    pLogBox -> textCursor().insertText("System Log:\n");
    pLogBox -> setFixedHeight(90);
    pLogBox -> setEnabled(false);

    pRightLayout -> addWidget(pRightCtrlInterface);
    pRightLayout -> addWidget(pRightCanvas);
    pRightLayout -> addWidget(pLogBox);
}


////////////////////////////////////////////////////////////////
// setup the control interface
// TODO: function too long, need to split

void Viewer::InitCtrlInterface()
{
    QVBoxLayout *layout = new QVBoxLayout(pRightCtrlInterface);

    // a file path input window
    QGridLayout *_layout1 = new QGridLayout();
    QLabel *l1 = new QLabel("File:", pRightCtrlInterface);
    file_indicator = new QLineEdit(pRightCtrlInterface);
    QPushButton *bOpenFile = new QPushButton("Choose &File", pRightCtrlInterface);
    file_indicator -> setText("gui/data/gem_cleanroom_1440.evio.0");
    _layout1 -> addWidget(l1, 0, 0);
    _layout1 -> addWidget(file_indicator, 0, 1);
    _layout1 -> addWidget(bOpenFile, 0, 2);

   // a event number input window
    QHBoxLayout *_layout2 = new QHBoxLayout();
    QLabel *l2 = new QLabel("Event Number: ", pRightCtrlInterface);
    QSpinBox *event_number = new QSpinBox(pRightCtrlInterface);
    event_number -> setRange(0, 9999);
    event_number -> setObjectName("event_number");
    QPushButton *btn_save_event = new QPushButton("Save Event to Disk", pRightCtrlInterface);
    _layout2 -> addWidget(l2);
    _layout2 -> addWidget(btn_save_event);
    _layout2 -> addWidget(event_number);

    // function modules for generating pedestals 
    QGridLayout *_layout3 = new QGridLayout();
    // 1) max event for pedestal
    QLabel *l_num = new QLabel("Max events for pedestal: ", pRightCtrlInterface);
    QLineEdit *le_num = new QLineEdit(pRightCtrlInterface);
    le_num -> setText("5000");
    // 2) pedestal output path
    QLabel *l_path = new QLabel("Pedestal Text File Output Path:", pRightCtrlInterface);
    QLineEdit *le_path = new QLineEdit(pRightCtrlInterface);
    le_path -> setText("database/gem_ped_1440.dat");
    le_path -> setObjectName("le_path");
    // 3) common mode range save path
    QLabel *l_commonMode = new QLabel("Commom Mode Range Table: ", pRightCtrlInterface);
    QLineEdit *le_commonMode = new QLineEdit(pRightCtrlInterface);
    le_commonMode -> setText("database/CommonModeRange_1440.txt");
    le_commonMode -> setObjectName("le_commonMode");
    //le_commonMode -> setEnabled(false);
    // 4) generate
    QLabel *l3 = new QLabel("Generate Pedestal/commonMode:", pRightCtrlInterface);
    QPushButton *b = new QPushButton("&Generate", pRightCtrlInterface);
    _layout3 -> addWidget(l_num, 0, 0);
    _layout3 -> addWidget(le_num, 0, 1);
    _layout3 -> addWidget(l_path, 1, 0);
    _layout3 -> addWidget(le_path, 1, 1);
    _layout3 -> addWidget(l_commonMode, 2, 0);
    _layout3 -> addWidget(le_commonMode, 2, 1);
    _layout3 -> addWidget(l3, 3, 0);
    _layout3 -> addWidget(b, 3, 1);

    // 5) set pedestal file and mapping file for replay
    QGridLayout *_layout7 = new QGridLayout();
    // pedestal
    QLabel *l_pedestal_for_replay = new QLabel("Load Pedestal File From: ", pRightCtrlInterface);
    QLineEdit *le_pedestal_for_replay = new QLineEdit(pRightCtrlInterface);
    fPedestalInputPath = txt_parser.Value<std::string>("GEM Pedestal");
    le_pedestal_for_replay -> setText(fPedestalInputPath.c_str());
    le_pedestal_for_replay -> setObjectName("le_pedestal_for_replay");
    QPushButton *btn_choose_pedestal = new QPushButton("Choose &Pedestal", pRightCtrlInterface);
    // common mode
    QLabel *l_common_mode_for_replay = new QLabel("Load Common Mode From:", pRightCtrlInterface);
    QLineEdit *le_common_mode_for_replay = new QLineEdit(pRightCtrlInterface);
    fCommonModeInputPath = txt_parser.Value<std::string>("GEM Common Mode");
    le_common_mode_for_replay -> setText(fCommonModeInputPath.c_str());
    le_common_mode_for_replay -> setObjectName("le_common_mode_for_replay");
    QPushButton *btn_choose_common_mode = new QPushButton("Choose Common &Mode", pRightCtrlInterface);
    // mapping
    QLabel *l_mapping = new QLabel("Load Mapping File From:", pRightCtrlInterface);
    QLineEdit *le_mapping = new QLineEdit(pRightCtrlInterface);
    le_mapping -> setText(txt_parser.Value<std::string>("GEM Map").c_str());
    QPushButton *btn_choose_mapping = new QPushButton("Choose &Mapping", pRightCtrlInterface);
    btn_choose_mapping -> setEnabled(false);
    _layout7 -> addWidget(l_pedestal_for_replay, 1, 0);
    _layout7 -> addWidget(le_pedestal_for_replay, 1, 1);
    _layout7 -> addWidget(btn_choose_pedestal, 1, 2);
    _layout7 -> addWidget(l_common_mode_for_replay, 2, 0);
    _layout7 -> addWidget(le_common_mode_for_replay, 2, 1);
    _layout7 -> addWidget(btn_choose_common_mode, 2, 2);
    _layout7 -> addWidget(l_mapping, 3, 0);
    _layout7 -> addWidget(le_mapping, 3, 1);
    _layout7 -> addWidget(btn_choose_mapping, 3, 2);

    // set file split
    QGridLayout *_layout6 = new QGridLayout();
    QLabel *l_split = new QLabel("File Split Range for Replay: ", pRightCtrlInterface);
    QLineEdit *le_split = new QLineEdit(pRightCtrlInterface);
    le_split -> setText("-1");
    QLineEdit *le_split_start = new QLineEdit(pRightCtrlInterface);
    le_split_start -> setText("0");
    _layout6 -> addWidget(l_split, 1, 0);
    _layout6 -> addWidget(le_split_start, 1, 1);
    _layout6 -> addWidget(le_split, 1, 2);

    // function moudles for replaying evio file to hit root files
    QGridLayout *_layout4 = new QGridLayout();
    QLabel *l_replay = new QLabel("Replay Hit File Output Path:", pRightCtrlInterface);
    QLineEdit *le_replay = new QLineEdit(pRightCtrlInterface);
    le_replay -> setText("Rootfiles/hit_[gem_cleanrom]_[run].root");
    le_replay -> setEnabled(false);
    QLabel *l4 = new QLabel("Replay to Hit ROOT file: ", pRightCtrlInterface);
    QPushButton *b4 = new QPushButton("GEM &Hit Replay", pRightCtrlInterface);
    _layout4 -> addWidget(l_replay, 0, 0);
    _layout4 -> addWidget(le_replay, 0, 1);
    _layout4 -> addWidget(l4, 1, 0);
    _layout4 -> addWidget(b4, 1, 1);

    // function modules for clustering, and save to root files
    QGridLayout *_layout5 = new QGridLayout();
    QLabel *l_cluster_path = new QLabel("Cluster File Output Path:", pRightCtrlInterface);
    QLineEdit *le_cluster_path = new QLineEdit(pRightCtrlInterface);
    le_cluster_path -> setText("Rootfiles/cluster_[gem_cleanroom]_[run].root");
    le_cluster_path -> setEnabled(false);
    QLabel *l_cluster = new QLabel("Clustering Replay:", pRightCtrlInterface);
    QPushButton *btn_cluster = new QPushButton("GEM &Cluster Replay", pRightCtrlInterface);
    _layout5 -> addWidget(l_cluster_path, 1, 0);
    _layout5 -> addWidget(le_cluster_path, 1, 1);
    _layout5 -> addWidget(l_cluster, 2, 0);
    _layout5 -> addWidget(btn_cluster, 2, 1);

    minimum_qt_unit_height(l1, file_indicator, bOpenFile, l2, btn_save_event, event_number,
            l_num, le_num, l_path, le_path, l_commonMode, le_commonMode, l3, b,
            l_pedestal_for_replay, le_pedestal_for_replay, btn_choose_pedestal,
            l_common_mode_for_replay, le_common_mode_for_replay, btn_choose_common_mode,
            l_mapping, le_mapping, btn_choose_mapping,
            l_split, le_split_start, le_split, l_replay, le_replay, l4, b4,
            l_cluster_path, le_cluster_path, l_cluster, btn_cluster);

    // add to overall layout
    layout -> addLayout(_layout1);
    layout -> addLayout(_layout2);
    layout -> addLayout(_layout3);
    layout -> addLayout(_layout7);
    layout -> addLayout(_layout6);
    layout -> addLayout(_layout4);
    layout -> addLayout(_layout5);

    // connect
    connect(file_indicator, SIGNAL(textChanged(const QString &)), this, SLOT(SetFile(const QString &)));
    connect(event_number, SIGNAL(valueChanged(int)), this, SLOT(DrawEvent(int)));
    connect(btn_save_event, SIGNAL(pressed()), this, SLOT(SaveCurrentEvent()));
    connect(bOpenFile, SIGNAL(pressed()), this, SLOT(OpenFile()));
    connect(b, SIGNAL(pressed()), this, SLOT(GeneratePedestal()));
    connect(le_path, SIGNAL(textChanged(const QString &)), this, SLOT(SetPedestalOutputPath(const QString &)));
    connect(le_commonMode, SIGNAL(textChanged(const QString &)), this, SLOT(SetCommonModeOutputPath(const QString &)));
    connect(le_num, SIGNAL(textChanged(const QString &)), this, SLOT(SetPedestalMaxEvents(const QString &)));
    connect(le_replay, SIGNAL(textChanged(const QString &)), this, SLOT(SetRootFileOutputPath(const QString &)));
    connect(le_split, SIGNAL(textChanged(const QString &)), this, SLOT(SetFileSplitMax(const QString &)));
    connect(le_split_start, SIGNAL(textChanged(const QString &)), this, SLOT(SetFileSplitMin(const QString &)));
    connect(b4, SIGNAL(pressed()), this, SLOT(ReplayHit()));
    connect(btn_cluster, SIGNAL(pressed()), this, SLOT(ReplayCluster()));
    connect(btn_choose_pedestal, SIGNAL(pressed()), this, SLOT(ChoosePedestal()));
    connect(btn_choose_common_mode, SIGNAL(pressed()), this, SLOT(ChooseCommonMode()));
    connect(le_pedestal_for_replay, SIGNAL(textChanged(const QString &)), this, SLOT(SetPedestalInputPath(const QString &)));
    connect(le_common_mode_for_replay, SIGNAL(textChanged(const QString &)), this, SLOT(SetCommonModeInputPath(const QString &)));
    // connect online analysis
    connect(pOpenAnalysisInterface, SIGNAL(triggered()), this, SLOT(OpenOnlineAnalysisInterface()));
}

 
////////////////////////////////////////////////////////////////
// setup the tabs in left drawing area

void Viewer::InitLeftTab()
{
    pLeftTab = new QTabWidget(pLeft);

    auto v_mpd_addr = apv_strip_mapping::Mapping::Instance()->GetMPDAddressVec();

    // for apv raw histos
    for(int i=0;i<nTab;i++) 
    {
        QWidget *tabWidget = new QWidget(pLeftTab);
        QVBoxLayout *tabWidgetLayout = new QVBoxLayout(tabWidget);
        HistoWidget *c = new HistoWidget(tabWidget);
        c -> PassQMainCanvasPointer(pRightCanvas);
        tabWidgetLayout -> addWidget(c);
        vTabCanvas.push_back(c);

        QString s = QString("slot %1 fiber %2").arg(v_mpd_addr[i].crate_id).arg(v_mpd_addr[i].mpd_id);
        pLeftTab -> addTab(tabWidget, s);
    }

    auto layer_id_vec = apv_strip_mapping::Mapping::Instance() -> GetLayerIDVec();

    // for gem chamber online hits
    for(int i=0; i<nTabOnlineHits; ++i)
    {
        QWidget *tabWidget = new QWidget(pLeftTab);
        QVBoxLayout *tabWidgetLayout = new QVBoxLayout(tabWidget);
        HistoWidget *c = new HistoWidget(tabWidget);
        c -> Divide(4, 2); // each layer has 4 chambers
        c -> PassQMainCanvasPointer(pRightCanvas);
        tabWidgetLayout -> addWidget(c);
        vTabCanvasOnlineHits.push_back(c);

        QString s = QString("Online Hits Layer: %1").arg(layer_id_vec[i]);
        pLeftTab -> addTab(tabWidget, s);
    }

    pLeftLayout -> addWidget(pLeftTab);

#ifdef EYE_BALL_TRACKING
    // for eye-ball tracking (show detector 2d strips)
    det_view = new Detector2DView();
    pLeftTab -> addTab(det_view, "Detector 2D Strips");
#endif
}


////////////////////////////////////////////////////////////////
// set how many tabs for showing histograms
// rely on mapping

void Viewer::SetNumberOfTabs()
{
    // for apv raw histos
    nTab = apv_strip_mapping::Mapping::Instance()->GetTotalMPDs();

    // for gem chamber online hits
    int number_of_layers = apv_strip_mapping::Mapping::Instance()->GetTotalNumberOfLayers();
    nTabOnlineHits = number_of_layers; // here assume each layer has 4 chambers max
}


////////////////////////////////////////////////////////////////
// set file to analyzer

void Viewer::SetFile([[maybe_unused]] const QString & s)
{
    fFile = s.toStdString();
    if(!FileExist(fFile.c_str()))
    {
        std::cout<<"Error:: cannot find data file: \""<<fFile<<"\"."
                 <<std::endl;
        return;
    }

    if(fFile.find("evio") == std::string::npos &&
            fFile.find("dat") == std::string::npos )
    {
        std::cout<<"Error:: only evio or dat file are accepted."
                 <<std::endl;
        return;
    }

    std::cout<<"Openning file: "<<fFile<<std::endl;

    pGEMAnalyzer -> CloseFile();
    pGEMAnalyzer -> SetFile(s.toStdString().c_str());
    pGEMAnalyzer -> Init();
}

////////////////////////////////////////////////////////////////
//  init gem analyzer

void Viewer::InitGEMAnalyzer()
{
    pGEMAnalyzer = new GEMAnalyzer();
    pGEMAnalyzer -> SetFile(fFile.c_str());
    pGEMAnalyzer -> Init();

    pGEMReplay = new GEMReplay();
}

////////////////////////////////////////////////////////////////
// draw event

void Viewer::DrawEvent(int num)
{
    // raw data will be fetched in DrawGEMRawHistos, 
    // thus DrawHistos() must be called first
    DrawGEMRawHistos(num);

    DrawGEMOnlineHits(num);
}


////////////////////////////////////////////////////////////////
// draw gem raw event

void Viewer::DrawGEMRawHistos(int num)
{
    std::map<APVAddress, std::vector<int>> mData;
    std::map<APVAddress, APVDataType> mDataFlags;

    // event number increased - forward
    if(num > event_number_checked)
    {
        // get apv raw histos
        pGEMAnalyzer -> AnalyzeEvent(num);
        auto & _mData = pGEMAnalyzer->GetData();
        auto & _mDataFlags = pGEMAnalyzer -> GetDataFlags();
        if(_mData.size() <= 0) return;

        // sort raw histos
        for(auto &i: _mData)
            mData[i.first] = i.second;
        // sort flags
        for(auto &i: _mDataFlags)
            mDataFlags[i.first] = i.second;

        event_number_checked = num;
        current_event_number = num;
        event_cache.push_back(mData);
        event_flag_cache.push_back(mDataFlags);
        if(event_cache.size() > max_cache_events) 
        {
            event_cache.pop_front();
            event_flag_cache.pop_front();
        }
    }
    // event number decreased - backward
    else if(num < event_number_checked) 
    {
        size_t index = event_number_checked - num;
        if(index >= event_cache.size())
                return;

        size_t pos = event_cache.size() - index - 1;
        current_event_number = num;

        mData = event_cache.at(pos);
    }
    else
    {
        if(event_cache.size() <= 0) return;
        mData = event_cache.back();
    }

    // print a log 
    std::string ss("total apv in current event : ");
    ss = ss + std::to_string(mData.size()) + "\n";
    pLogBox -> textCursor().insertText(ss.c_str());
    pLogBox -> verticalScrollBar()->setValue(pLogBox->verticalScrollBar()->maximum());

    // dispath by mpd id
    // a helper
    auto find_index = [&](const MPDAddress &addr) -> int 
    {
        auto &vMPDAddr = apv_strip_mapping::Mapping::Instance()->GetMPDAddressVec();
        int res = 0;
        for(auto &i: vMPDAddr) {
            if(i == addr)
                return res;
            res++;
        }
        return -1;
    };
    std::vector<std::vector<int>> vH[nTab];
    std::vector<APVAddress> vAddr[nTab];
    for(auto &i: mData) {
        MPDAddress mpd_addr(i.first.crate_id, i.first.mpd_id);
        int index = find_index(mpd_addr);
        if(index >= 0 && index < nTab) {
            vH[index].push_back(i.second);
            vAddr[index].push_back(i.first);
        }
    }

    // draw tab main canvas
    for(int i=0;i<nTab;i++) 
    {
        vTabCanvas[i] -> Clear();
        vTabCanvas[i] -> DrawCanvas(vH[i], vAddr[i], 4, 4);
        vTabCanvas[i] -> Refresh();
    }

    // draw right-side canvas
    std::vector<std::vector<int>> temp;
    std::vector<APVAddress> temp_addr;
    temp.push_back(mData.begin()->second);
    temp_addr.push_back(mData.begin()->first);
    pRightCanvas->DrawCanvas(temp, temp_addr, 1, 1);
}


////////////////////////////////////////////////////////////////
// draw extracted gem online hits (fired strips after zero
// suppression for each GEM Chamber)

void Viewer::DrawGEMOnlineHits(int num)
{
    if(reload_pedestal_for_online) {
        std::cout<<"loading pedestal for online analysis from: \""
                 <<fPedestalInputPath
                 <<"\" and \""<<fCommonModeInputPath<<"\""<<std::endl;
        pGEMReplay -> GetGEMSystem() -> ReadPedestalFile(fPedestalInputPath, fCommonModeInputPath);
        reload_pedestal_for_online = false;
    }

    if(event_cache.size() <= 0)
        return;

    // get raw data
    std::map<APVAddress, std::vector<int>> event_data;
    std::map<APVAddress, APVDataType> event_data_flag;
    if( num > event_number_checked)
    {
        event_data = event_cache.back();
        event_data_flag = event_flag_cache.back();
    }
    else if( num < event_number_checked)
    {
        size_t index = event_number_checked - num;
        if(index >= event_cache.size())
            return;
        size_t pos = event_cache.size() - index - 1;
        event_data = event_cache.at(pos);
        event_data_flag = event_flag_cache.at(pos);
    }
    else
    {
        if(event_cache.size() <= 0) return;
        event_data = event_cache.back();
        event_data_flag = event_flag_cache.back();
    }

    // online zero suppression
    for(auto &i: event_data)
    {
        GEMAPV *apv = pGEMReplay -> GetGEMSystem() -> GetAPV(i.first);
        if( apv == nullptr)
        {
            std::cout<<__func__<<": Warning: apv "<<i.first<<" not initilized"
                     <<std::endl
                     <<"            make sure mapping file is correct."
                     <<std::endl
                     <<"            skipped current apv data."
                     <<std::endl;
            continue;
        }

        apv -> FillRawDataMPD(i.second, event_data_flag.at(i.first));
        apv -> ZeroSuppression();
        apv -> CollectZeroSupHits();
    }

    // organize online hits by layer
    auto & layerID = apv_strip_mapping::Mapping::Instance() -> GetLayerIDVec();

    // online hits contents
    // (x_hits, y_hits)[layer][chamber]
    std::pair<std::vector<int>, std::vector<int>> online_hits[layerID.size()][4];

    // get all gem detectors
    std::vector<GEMDetector*> detectorList = 
        pGEMReplay -> GetGEMSystem() -> GetDetectorList();

    // a helper to draw histos
    auto get_histo = [&](const std::vector<StripHit> &hits, const int &nAPV, bool xPlane,
            const int &GEMPos) -> std::vector<int>
    {
        std::vector<int> res(nAPV * APV_STRIP_SIZE + 10, 0); // +10 is for safety reason
        for(auto &i: hits) {
            int hit_pos = i.strip;
            if(xPlane) { // x plane need to go to local GEM coord
                hit_pos -= APV_STRIP_SIZE * GEMPos * nAPV;
            }
            if(hit_pos >= nAPV * APV_STRIP_SIZE)
                std::cout<<"ERROR: strip no: "<<hit_pos<<", vector size: "
                    <<nAPV * APV_STRIP_SIZE<<std::endl;
            res[hit_pos] = static_cast<int>(i.charge);
        }
        return res;
    };
    // a helper to get layer_id position in vector
    // (due to the true layer_id may not start from 0, so layerID vector may not have 0)
    auto get_vector_index = [&](const int &layer_id) -> int
    {
        for(size_t i=0; i<layerID.size();++i)
        {
            if(layer_id == layerID[i])
                return i;
        }
        return -1;
    };

    // extract all hits
    for(auto &i: detectorList)
    {
        int layer_id = i -> GetLayerID();
        int chamber_pos = i -> GetDetLayerPositionIndex();

        // chamber pos should be 0 - 3, max 4 chambers in one layer
        if(chamber_pos <0 || chamber_pos > 3)
            continue;

        GEMPlane *pln_x = i -> GetPlane(GEMPlane::Plane_X);
        GEMPlane *pln_y = i -> GetPlane(GEMPlane::Plane_Y);

        const std::vector<StripHit> & x_hits = pln_x -> GetStripHits();
        int x_apvs = pln_x -> GetCapacity();
        std::vector<int> x_online_hits = get_histo(x_hits, x_apvs, true, chamber_pos);

        const std::vector<StripHit> & y_hits = pln_y -> GetStripHits();
        int y_apvs = pln_y -> GetCapacity();
        std::vector<int> y_online_hits = get_histo(y_hits, y_apvs, false, chamber_pos);

        int index = get_vector_index(layer_id);

        // layer id not found in mapping file
        if(index < 0)
            continue;

        online_hits[index][chamber_pos] = std::pair<std::vector<int>,
            std::vector<int>>(x_online_hits, y_online_hits);

        // clear strip hits for next event
        pln_x -> ClearStripHits();
        pln_y -> ClearStripHits();
    }

    // draw online hits
    for(size_t i=0; i<layerID.size(); ++i)
    {
        std::vector<std::vector<int>> data;
        std::vector<std::string> title;
        // 4 chambers
        for(int j=0; j<4; ++j)
        {
            data.push_back(online_hits[i][j].first);
            std::string _tmpx = "Layer " + std::to_string(i) + " Chamber " + 
                               std::to_string(j) + " X Plane";
            title.push_back(_tmpx);
            data.push_back(online_hits[i][j].second);
            std::string _tmpy = "Layer " + std::to_string(i) + " Chamber " + 
                               std::to_string(j) + " Y Plane";
            title.push_back(_tmpy);
        }

        vTabCanvasOnlineHits[i] -> Clear();
        vTabCanvasOnlineHits[i] -> DrawCanvas(data, title, 4, 2);
        vTabCanvasOnlineHits[i] -> Refresh();
    }

#ifdef EYE_BALL_TRACKING
    // draw eye-ball tracking GEM 2D strips
    det_view -> FillEvent(online_hits);
#endif
}


////////////////////////////////////////////////////////////////
// save current event

void Viewer::SaveCurrentEvent()
{
    std::string file_name = "./Rootfiles/event_" + 
        std::to_string(current_event_number) + ".txt";
    std::cout<<file_name<<std::endl;

    std::map<APVAddress, std::vector<int>> _event;
    if(current_event_number == event_number_checked)
    {
        _event = event_cache.back();
    }
    else
    {
        size_t index = event_number_checked - current_event_number;
        if(index >= event_cache.size())
                return;
        size_t pos = event_cache.size() - index - 1;

        _event = event_cache.at(pos);
    }

    std::fstream f(file_name.c_str(), std::fstream::out);
    for(auto &i: _event) {
        f<<"apv:"<<i.first;
        for(auto &j: i.second)
            f<<j<<std::endl;
    }
}


////////////////////////////////////////////////////////////////
// check if file exists

bool Viewer::FileExist(const char* path)
{
    std::ifstream infile(path);
    return infile.good();
}

////////////////////////////////////////////////////////////////
// open file dialog

void Viewer::OpenFile()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open Document",
            //QDir::currentPath(),
            "/home/daq/coda/data",
            "All files (*.*) ;; evio files (*.evio)");

    fFile = filename.toStdString();

    if(fFile.size() <= 0) 
        return;

    file_indicator -> setText(filename);

    // reset event counter to 0
    pRightCtrlInterface -> findChild<QSpinBox*>(QString("event_number"))
        -> setValue(0);
    event_number_checked = 0;

    // update pedstal output path
    ParsePedestalsOutputPathFromEvioFile();
}

////////////////////////////////////////////////////////////////
// parse pedestals output path based on input evio file

void Viewer::ParsePedestalsOutputPathFromEvioFile()
{
    int run_number = InfoCenter::Instance()->ParseRunNumber(fFile);
    fPedestalOutputPath = "database/gem_ped_" + std::to_string(run_number)
        + ".dat";
    fCommonModeOutputPath = "database/CommonModeRange_" + std::to_string(run_number)
        + ".txt";

    pRightCtrlInterface -> findChild<QLineEdit*>(QString("le_path"))
        -> setText(fPedestalOutputPath.c_str());
    pRightCtrlInterface -> findChild<QLineEdit*>(QString("le_commonMode"))
        -> setText(fCommonModeOutputPath.c_str());
}

////////////////////////////////////////////////////////////////
// set pedestal output file path

void Viewer::SetPedestalOutputPath(const QString &s)
{
    fPedestalOutputPath = s.toStdString();
}

////////////////////////////////////////////////////////////////
// set common mode output file path

void Viewer::SetCommonModeOutputPath(const QString &s)
{
    fCommonModeOutputPath = s.toStdString();
}

////////////////////////////////////////////////////////////////
// set pedestal input file path

void Viewer::SetPedestalInputPath(const QString &s)
{
    fPedestalInputPath = s.toStdString();

    // reset online pedestal
    reload_pedestal_for_online = true;
}

////////////////////////////////////////////////////////////////
// set common mode input file path

void Viewer::SetCommonModeInputPath(const QString &s)
{
    fCommonModeInputPath = s.toStdString();

    // reset online pedestal
    reload_pedestal_for_online = true;
}

////////////////////////////////////////////////////////////////
// choose pedestal file for data analysis

void Viewer::ChoosePedestal()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open Document",
            QDir::currentPath(),
            "All files (*.*) ;; evio files (*.evio)");

    if(filename.size() <= 0)
        return;

    fPedestalInputPath = filename.toStdString();

    // reset online pedestal
    reload_pedestal_for_online = true;

    if(fPedestalInputPath.size() <= 0)
        fPedestalInputPath = "database/gem_ped.dat";

    pRightCtrlInterface -> findChild<QLineEdit*>(QString("le_pedestal_for_replay"))
            -> setText(filename);
}

////////////////////////////////////////////////////////////////
// choose common mode file for data analysis

void Viewer::ChooseCommonMode()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open Document",
            QDir::currentPath(),
            "All files (*.*) ;; evio files (*.evio)");

    if(filename.size() <= 0)
        return;

    fCommonModeInputPath = filename.toStdString();

    // reset online pedestal
    reload_pedestal_for_online = true;

    if(fCommonModeInputPath.size() <= 0)
        fCommonModeInputPath = "database/CommonModeRange.txt";

    pRightCtrlInterface -> findChild<QLineEdit*>(QString("le_common_mode_for_replay"))
            -> setText(filename);
}

////////////////////////////////////////////////////////////////
// set root file output path

void Viewer::SetRootFileOutputPath(const QString &s)
{
    fRootFileSavePath = s.toStdString();
}

////////////////////////////////////////////////////////////////
// set input file split max [min, max]

void Viewer::SetFileSplitMax(const QString &s)
{
    std::string ss = s.toStdString();
    int i = std::stoi(ss);

    fFileSplitEnd = i;
}

////////////////////////////////////////////////////////////////
// set input file split min [min, max]

void Viewer::SetFileSplitMin(const QString &s)
{
    std::string ss = s.toStdString();
    int i = std::stoi(ss);

    fFileSplitStart = i;
}

////////////////////////////////////////////////////////////////
// set pedestal max events

void Viewer::SetPedestalMaxEvents(const QString & s)
{
    std::string ss = s.toStdString();
    int i = std::stoi(ss);
    if(i <= 0) {
        std::cout<<"Warning: max events for generating pedestal is invalid"
                 <<std::endl;
        std::cout<<"     using default 5000."<<std::endl;
        return;
    }
    fPedestalMaxEvents = i;
}


////////////////////////////////////////////////////////////////
// generate pedestal (obsolete)

void Viewer::GeneratePedestal_obsolete()
{
    QMessageBox::StandardButton reply = QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Generating Pedestal/CommonMode usually take ~30 seconds. \
                \nPress Yes to start..."),
            QMessageBox::No | QMessageBox::Yes);
    if(reply != QMessageBox::Yes)
        return;

    pLogBox -> setTextColor(QColor("blue"));
    pLogBox -> textCursor().insertText("\ngenerating pedestal/commonMode for file: \"");
    pLogBox -> textCursor().insertText(fFile.c_str());
    pLogBox -> textCursor().insertText("\"\nthis might take a while...\n");
    pLogBox -> verticalScrollBar()->setValue(pLogBox->verticalScrollBar()->maximum());
    QCoreApplication::processEvents();

    //pGEMAnalyzer -> GeneratePedestal(fPedestalOutputPath.c_str());

    std::thread th([&]() {
            pGEMAnalyzer -> GeneratePedestal(fPedestalOutputPath.c_str());
            }
            );

    th.join();

    QMessageBox::information(
            this,
            tr("MPD GEM Viewer"),
            tr("Pedestal/CommonMode Done!") );

    pLogBox -> setTextColor(QColor("black"));
    pLogBox -> textCursor().insertText("Done.\n");
}

////////////////////////////////////////////////////////////////
// generate pedestal

void Viewer::GeneratePedestal()
{
    QMessageBox::StandardButton reply = QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Generating Pedestal/CommonMode usually takes a while... \
                \nPress Yes to start..."),
           QMessageBox::No | QMessageBox::Yes );
    if(reply != QMessageBox::Yes)
        return;

    pLogBox -> setTextColor(QColor("blue"));
    pLogBox -> textCursor().insertText("\ngenerating pedestal/commonMode for file: \"");
    pLogBox -> textCursor().insertText(fFile.c_str());
    pLogBox -> textCursor().insertText("\"\nthis might take a while...\n");
    pLogBox -> verticalScrollBar()->setValue(pLogBox->verticalScrollBar()->maximum());
    QCoreApplication::processEvents();

    pGEMReplay -> SetInputFile(fFile);
    pGEMReplay -> SetPedestalOutputFile(fPedestalOutputPath);
    pGEMReplay -> SetCommonModeOutputFile(fCommonModeOutputPath);
    pGEMReplay -> SetPedestalInputFile(fPedestalInputPath, fCommonModeInputPath);
    pGEMReplay -> SetSplitMax(fFileSplitEnd);
    pGEMReplay -> SetSplitMin(fFileSplitStart);
    pGEMReplay -> SetMaxPedestalEvents(fPedestalMaxEvents);


    std::thread th([&]() {
            pGEMReplay -> GeneratePedestal();
            }
            );

    th.join();

    QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("\nPedestal file written to: %1\
                \nCommonMode file written to: %2\
                \n\nIf you want to use the new files, please set GUI section \"Choose Pedestal\" accordingly.\
                \n\nPedestal/CommonMode Done!")
            .arg(fPedestalOutputPath.c_str())
            .arg(fCommonModeOutputPath.c_str())
            );

    pLogBox -> setTextColor(QColor("black"));
    pLogBox -> textCursor().insertText("Done.\n");
}

////////////////////////////////////////////////////////////////
// replay hit 

void Viewer::ReplayHit()
{
    QMessageBox::StandardButton reply = QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Replay files usually takes a while... \
                \nPress Yes to start..."),
            QMessageBox::No | QMessageBox::Yes );
    if(reply != QMessageBox::Yes)
        return;

    pLogBox -> setTextColor(QColor("blue"));
    pLogBox -> textCursor().insertText("\nReplaying for file: \"");
    pLogBox -> textCursor().insertText(fFile.c_str());
    pLogBox -> textCursor().insertText("\"\nthis might take a while...\n");
    pLogBox -> verticalScrollBar()->setValue(pLogBox->verticalScrollBar()->maximum());
    QCoreApplication::processEvents();

    pGEMReplay -> SetInputFile(fFile);
    pGEMReplay -> SetPedestalOutputFile(fPedestalOutputPath);
    pGEMReplay -> SetPedestalInputFile(fPedestalInputPath, fCommonModeInputPath);
    pGEMReplay -> SetCommonModeOutputFile(fCommonModeOutputPath);
    //pGEMReplay -> SetOutputFile(fRootFileSavePath); // now code automatically deduct output path
    pGEMReplay -> SetSplitMax(fFileSplitEnd);
    pGEMReplay -> SetSplitMin(fFileSplitStart);

    std::thread th([&]() {
            pGEMReplay -> ReplayHit();
            }
            );

    th.join();

    QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Replay Done!") );

    pLogBox -> setTextColor(QColor("black"));
    pLogBox -> textCursor().insertText("Done.\n");
}


////////////////////////////////////////////////////////////////
// replay cluster

void Viewer::ReplayCluster()
{
    QMessageBox::StandardButton reply = QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Replay files usually takes a while... \
                \nPress Yes to start..."),
            QMessageBox::No | QMessageBox::Yes );
    if(reply != QMessageBox::Yes)
        return;

    pLogBox -> setTextColor(QColor("blue"));
    pLogBox -> textCursor().insertText("\nReplaying for file: \"");
    pLogBox -> textCursor().insertText(fFile.c_str());
    pLogBox -> textCursor().insertText("\"\nthis might take a while...\n");
    pLogBox -> verticalScrollBar()->setValue(pLogBox->verticalScrollBar()->maximum());
    QCoreApplication::processEvents();

    pGEMReplay -> SetInputFile(fFile);
    pGEMReplay -> SetPedestalOutputFile(fPedestalOutputPath);
    pGEMReplay -> SetPedestalInputFile(fPedestalInputPath, fCommonModeInputPath);
    pGEMReplay -> SetCommonModeOutputFile(fCommonModeOutputPath);
    //pGEMReplay -> SetOutputFile(fRootFileSavePath); // now code automatically deduct output path
    pGEMReplay -> SetSplitMax(fFileSplitEnd);
    pGEMReplay -> SetSplitMin(fFileSplitStart);

    std::thread th([&]() {
            pGEMReplay -> ReplayCluster();
            }
            );

    th.join();

    QMessageBox::information(
            this,
            tr("GEM Data Viewer"),
            tr("Replay Done!") );

    pLogBox -> setTextColor(QColor("black"));
    pLogBox -> textCursor().insertText("Done.\n");
}

void Viewer::OpenOnlineAnalysisInterface()
{
    winOnlineInterface = new OnlineAnalysisInterface();
    winOnlineInterface -> show();
}
