#include "Viewer.h"
#include "InfoCenter.h"
#include "APVStripMapping.h"
#include "hardcode.h"
#include "experiment_setup/PRadSetup.h"

#include <QGraphicsRectItem>
#include <QSpinBox>
#include <QLabel>
#include <QTabWidget>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QCoreApplication>
#include <QTimer>
#include <QSplitter>
#include <QStackedWidget>
#include <QGroupBox>
#include <QToolBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QButtonGroup>

#include <iostream>
#include <fstream>
#include <thread>

//#define SHOW_APV_BY_MPD
#define APVS_PER_TAB_X 6
#define APVS_PER_TAB_Y 6
#define EYE_BALL_TRACKING
#define SHOW_PRAD_SETUP

////////////////////////////////////////////////////////////////
// ctor

Viewer::Viewer(QWidget *parent) : QMainWindow(parent)
{
    LoadMappingFile();
    InitGEMAnalyzer();
    InitGuiInterface();
    resize(sizeHint());
}

////////////////////////////////////////////////////////////////
// load mapping file for detector setup

void Viewer::LoadMappingFile()
{
    if(!txt_parser.ReadConfigFile("config/gem.conf")) {
        std::cout<<__func__<<" failed loading config file."<<std::endl;
        exit(1);
    }
}

////////////////////////////////////////////////////////////////
// init gui

void Viewer::InitGuiInterface()
{
    SetNumberOfTabs();

    // menu bar
    createMenuBar();

    // central widget
    QWidget *central = new QWidget(this);
    QVBoxLayout *vMain = new QVBoxLayout(central);
    vMain -> setContentsMargins(8, 8, 8, 8);
    vMain -> setSpacing(8);

    // Top toolbar
    QWidget *topToolbar = createTopToolbar();
    vMain -> addWidget(topToolbar);

    // central area: detector panel + settings panel
    QSplitter *splitter = new QSplitter(Qt::Horizontal, central);
    splitter -> addWidget(createDetectorPanel());
    splitter -> addWidget(createSettingsPanel());
    splitter -> setStretchFactor(0, 4);
    splitter -> setStretchFactor(1, 3);

    vMain -> addWidget(splitter, 1); // expanding

    // system log
    vMain -> addWidget(createSystemLogPanel());

    setCentralWidget(central);
    setWindowTitle(tr("GEM Data Viewer"));

    setStyleSheet(R"(
            QMainWindow {
                 background: #FFFFEF;
            }
            QGroupBox {
                 border: 1px solid #D0D3D8;
                 border-radius: 6px;
                 margin-top: 12px;
                 background: #FFFFFF;
            }
            QGroupBox::title {
                 subcontrol-origin: margin;
                 left: 10px;
            }
            QToolBox::tab {
                 background: #E8EAED;
                 border-radius: 4px;
                 padding: 0px, 0px;
                 margin: 2px;
            }
            QPlainTextEdit {
                background: #FFFFFF;
                color: #000000;
                border-radius: 0px;
                max-height: 40px;
            }
            )");
}

////////////////////////////////////////////////////////////////
// set how many tabs for showing histograms -- use input from mapping

void Viewer::SetNumberOfTabs()
{
    // for apv raw histos - show APV raw frames by MPD, each MPD takes a tab
#ifdef SHOW_APV_BY_MPD
    nTab = apv_strip_mapping::Mapping::Instance()->GetTotalMPDs();
#else
    // show fixed number of APVs per tab
    int total_apvs = apv_strip_mapping::Mapping::Instance()->GetTotalNumberOfAPVs();
    int apvs_per_tab = APVS_PER_TAB_X * APVS_PER_TAB_Y;
    nTab = (total_apvs + apvs_per_tab - 1)/apvs_per_tab;
#endif

    // for gem chamber online hits
    int number_of_layers = apv_strip_mapping::Mapping::Instance()->GetTotalNumberOfLayers();
    nTabOnlineHits = number_of_layers; // here assume each layer has 4 chambers max
}

////////////////////////////////////////////////////////////////
// top toolbar
QWidget* Viewer::createTopToolbar()
{
    QWidget *w = new QWidget(this);
    QHBoxLayout *h = new QHBoxLayout(w);

    // file slector
    QLabel *fileLabel = new QLabel(tr("File:"), w);
    m_fileCombo = new QComboBox(w);
    m_fileCombo -> setEditable(true);
    m_fileCombo -> setMinimumWidth(400);

    QPushButton *browseBtn = new QPushButton(tr("Browse..."), w);

    // view selector
    QLabel *viewLabel = new QLabel(tr("View:"), w);
    m_viewCombo = new QComboBox(w);
    m_viewCombo -> addItems({
            tr("APV Raw Frames"),
            tr("Online Hits"),
            tr("Detector 2D Strips")});

    // Event Selector
    QLabel *evtLabel = new QLabel(tr("Event:"), w);
    m_eventSpin = new QSpinBox(w);
    m_eventSpin -> setRange(0, 999999);
    m_eventSpin -> setValue(0);

    // save event button
    QPushButton *saveEvtBtn = new QPushButton(tr("Save Event"), w);

    h -> addWidget(fileLabel);
    h -> addWidget(m_fileCombo);
    h -> addWidget(browseBtn);
    h -> addSpacing(16);
    h -> addWidget(viewLabel);
    h -> addWidget(m_viewCombo);
    h -> addStretch(1);
    h -> addWidget(evtLabel);
    h -> addWidget(m_eventSpin);
    h -> addWidget(saveEvtBtn);

    // signal-slot
    connect(m_fileCombo, &QComboBox::currentTextChanged, this, &Viewer::SetFile);
    connect(browseBtn, &QPushButton::clicked, this, &Viewer::OpenFile);
    connect(m_eventSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &Viewer::DrawEvent);
    connect(saveEvtBtn, &QPushButton::pressed, this, &Viewer::SaveCurrentEvent);

    return w;
}

////////////////////////////////////////////////////////////////
// add a menu bar

void Viewer::createMenuBar()
{
    QMenuBar* pMenuBar = new QMenuBar();

    // file menu
    QMenu* pMenu = new QMenu("File");
    pMenu -> addAction("Save");
    pMenu -> addAction("Exit");
    pMenuBar -> addMenu(pMenu);

    // view menu
    pMenuBar -> addMenu(new QMenu("View"));
    // Edit menu
    pMenuBar -> addMenu(new QMenu("Edit"));
    // Format menu
    pMenuBar -> addMenu(new QMenu("Format"));

    // online analysis
    QMenu* pOnlineAnalysis = new QMenu("Online Analysis");
    QAction *pOpenAnalysisInterface = new QAction("Interface", this);
    pOnlineAnalysis -> addAction(pOpenAnalysisInterface);
    pMenuBar -> addMenu(pOnlineAnalysis);

    // Help Menu
    pMenuBar -> addMenu(new QMenu("Help"));

    connect(pOpenAnalysisInterface, &QAction::triggered, this, &Viewer::OpenOnlineAnalysisInterface);

    this->setMenuBar(pMenuBar);
}

////////////////////////////////////////////////////////////////
// setup the tabs in main drawing area
QWidget* Viewer::createDetectorPanel()
{
    QGroupBox *box = new QGroupBox(tr("GEM Detector Data"), this);
    QVBoxLayout *v = new QVBoxLayout(box);
    v -> setContentsMargins(2, 2, 2, 2);
    v -> setSpacing(2);

    QStackedWidget *w = new QStackedWidget(box);
    w -> addWidget(createRawFramesView(w));
    w -> addWidget(createOnlineHitsView(w));
    w -> addWidget(createDetector2DStripsView(w));

    v -> addWidget(w, 1);

    // signal-slot
    connect(m_viewCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            w, &QStackedWidget::setCurrentIndex);

    return box;
}

////////////////////////////////////////////////////////////////
// setup the tabs in main drawing area

QWidget* Viewer::createRawFramesView(QWidget *w)
{
    QTabWidget* tabW = new QTabWidget(w);

    // for apv raw histos
    for(int i=0;i<nTab;i++) 
    {
        QWidget *w = new QWidget(tabW);
        QVBoxLayout *wLayout = new QVBoxLayout(w);
        wLayout -> setContentsMargins(0, 0, 0, 0);
        wLayout -> setSpacing(0);
        HistoWidget *c = new HistoWidget(w);
        c -> Divide(APVS_PER_TAB_X, APVS_PER_TAB_Y);
        wLayout -> addWidget(c);
        vTabCanvas.push_back(c);

        QString s = QString("APV Raw Frames:") + QString::number(i);
        tabW -> addTab(w, s);
    }

    return tabW;
}

////////////////////////////////////////////////////////////////
// setup the tabs in left drawing area

QWidget* Viewer::createOnlineHitsView(QWidget *w)
{
    QTabWidget* tabW = new QTabWidget(w);

    auto layer_id_vec = apv_strip_mapping::Mapping::Instance() -> GetLayerIDVec();

    // for gem chamber online hits
    for(int i=0; i<nTabOnlineHits; ++i)
    {
        QWidget *w = new QWidget(tabW);
        QVBoxLayout *wLayout = new QVBoxLayout(w);
        wLayout -> setContentsMargins(0, 0, 0, 0);
        wLayout -> setSpacing(0);
        HistoWidget *c = new HistoWidget(w);
        c -> Divide(4, 2); // each layer has 4 chambers
        wLayout -> addWidget(c);
        vTabCanvasOnlineHits.push_back(c);

        QString s = QString("Online Hits Layer: %1").arg(layer_id_vec[i]);
        tabW -> addTab(w, s);
    }

    return tabW;
}

////////////////////////////////////////////////////////////////
// setup the tabs in left drawing area

QWidget* Viewer::createDetector2DStripsView(QWidget *w)
{
    QTabWidget* tabW = new QTabWidget(w);

#ifdef EYE_BALL_TRACKING
    // for eye-ball tracking (show detector 2d strips)
    det_view = new Detector2DView();
    tabW -> addTab(det_view, "Detector 2D Strips");
#endif

    return tabW;
}


////////////////////////////////////////////////////////////////
// init right drawing area for analysis settings

QWidget* Viewer::createSettingsPanel()
{
    QGroupBox *box = new QGroupBox(tr("Settings"), this);
    QVBoxLayout *v = new QVBoxLayout(box);
    v -> setContentsMargins(8, 8, 8, 8);
    v -> setSpacing(4);

    QToolBox *toolBox = new QToolBox(box);
#ifdef SHOW_PRAD_SETUP
    toolBox -> addItem(createPRadSetupPage(), tr("PRad GEM Setup"));
#endif
    toolBox -> addItem(createPedestalCommonModePage(), tr("Pedestal / Common Mode"));
    toolBox -> addItem(createMappingFilePage(), tr("Mapping File"));
    toolBox -> addItem(createReplayPage(), tr("Replay"));
    toolBox -> addItem(createAdvancedPage(), tr("Advanced"));

    v -> addWidget(toolBox, 1);
    return box;
}

////////////////////////////////////////////////////////////////
// set up common mode / pedestal
QWidget* Viewer::createPRadSetupPage()
{
    QWidget *page = new QWidget(this);
    QHBoxLayout *v = new QHBoxLayout(page);
    v -> setContentsMargins(0, 0, 0, 0);
    v -> setSpacing(20);

    PRadSetup *layer1 = new PRadSetup(page, 1);
    PRadSetup *layer2 = new PRadSetup(page, 2);
    v -> addWidget(layer1);
    v -> addWidget(layer2);

    // connect signals
    connect(this, &Viewer::onlineHitsDrawn, layer1, &PRadSetup::DrawEventHits2D);
    connect(this, &Viewer::onlineHitsDrawn, layer2, &PRadSetup::DrawEventHits2D);
 
    return page;
}

////////////////////////////////////////////////////////////////
// set up common mode / pedestal

QWidget* Viewer::createPedestalCommonModePage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *v = new QVBoxLayout(page);

    QGroupBox *pedBox = new QGroupBox(tr("Pedestal"), page);
    QFormLayout *pedForm = new QFormLayout(pedBox);
    pedForm -> setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QSpinBox *maxEvt = new QSpinBox(pedBox);
    maxEvt -> setRange(0, 100000000);
    maxEvt -> setValue(5000);
    m_pedOut = new QLineEdit(pedBox);
    m_pedOut -> setText(QString::fromStdString(txt_parser.Value<std::string>("GEM Pedestal")));
    m_cmOut = new QLineEdit(pedBox);
    m_cmOut -> setText(QString::fromStdString(txt_parser.Value<std::string>("GEM Common Mode")));

    QWidget *btnRow = new QWidget(pedBox);
    QHBoxLayout *h1 = new QHBoxLayout(btnRow);
    h1 -> setContentsMargins(0, 0, 0, 0);
    h1 -> setSpacing(4);
    QPushButton *genPed = new QPushButton(tr("Generate"), btnRow);
    QPushButton *loadPed = new QPushButton(tr("Load"), btnRow);
    h1 -> addWidget(genPed);
    h1 -> addWidget(loadPed);
    h1 -> addStretch(1);

    pedForm -> addRow(tr("Max Events:"), maxEvt);
    pedForm -> addRow(tr("Pedestal:"), m_pedOut);
    pedForm -> addRow(tr("CommonMode:"), m_cmOut);
    pedForm -> addRow(QString(), btnRow);

    v -> addWidget(pedBox);
    v -> addStretch(1);

    // signal-slot
    connect(genPed, &QPushButton::pressed, this, &Viewer::GeneratePedestal);
    connect(loadPed, &QPushButton::pressed, this, &Viewer::ReloadPedestal);
    connect(m_pedOut, &QLineEdit::textChanged, this, &Viewer::SetPedestalOutputPath);
    connect(m_cmOut, &QLineEdit::textChanged, this, &Viewer::SetCommonModeOutputPath);
    connect(maxEvt, QOverload<int>::of(&QSpinBox::valueChanged), this, &Viewer::SetPedestalMaxEvents);

    return page;
}

////////////////////////////////////////////////////////////////
// set up mapping file

QWidget* Viewer::createMappingFilePage()
{
    QWidget *page = new QWidget(this);
    QFormLayout *form = new QFormLayout(page);
    form -> setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QLineEdit *mapEdit = new QLineEdit(page);
    mapEdit -> setText(QString::fromStdString(txt_parser.Value<std::string>("GEM Map")));
    QPushButton *btn = new QPushButton(tr("Choose..."), page);

    form -> addRow(tr("Mapping File:"), mapEdit);
    form -> addRow(QString(), btn);

    // signal-slot
    connect(btn, &QPushButton::pressed, this, [this]() {
            m_logEdit->appendPlainText("[info] mapping file should be set in the config file: config/gem.conf");
            });

    return page;
}

////////////////////////////////////////////////////////////////
// set up root replay

QWidget* Viewer::createReplayPage()
{
    QWidget *page = new QWidget(this);
    QFormLayout *form = new QFormLayout(page);
    form -> setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QLineEdit *hitOut = new QLineEdit(page);
    hitOut -> setPlaceholderText("Rootfiles/hit_[prefix]_[run].root");
    QLineEdit *clusterOut = new QLineEdit(page);
    clusterOut -> setPlaceholderText("Rootfiles/cluster_[prefix]_[run].root");

    QPushButton *hitReplayBtn = new QPushButton(tr("Run Hit Replay"), page);
    QPushButton *clusterReplayBtn = new QPushButton(tr("Run Cluster Replay"), page);

    QSpinBox *splitFrom = new QSpinBox(page);
    QSpinBox *splitTo = new QSpinBox(page);
    splitFrom -> setRange(0, 1000000);
    splitTo -> setRange(-1, 1000000);
    splitTo -> setValue(-1);

    form -> addRow(tr("Hit Output"), hitOut);
    form -> addRow(tr("Cluster Output"), clusterOut);
    form -> addRow(tr("File Split From"), splitFrom);
    form -> addRow(tr("File Split To"), splitTo);
    form -> addRow(QString(), hitReplayBtn);
    form -> addRow(QString(), clusterReplayBtn);

    connect(hitOut, &QLineEdit::textChanged, this, &Viewer::SetRootFileOutputPath);
    //connect(clusterOut, &QLineEdit::textChanged, this, &Viewer::SetRootFileOutputPath);
    connect(splitFrom, QOverload<int>::of(&QSpinBox::valueChanged), this, &Viewer::SetFileSplitMin);
    connect(splitTo, QOverload<int>::of(&QSpinBox::valueChanged), this, &Viewer::SetFileSplitMax);
    connect(hitReplayBtn, &QPushButton::pressed, this, &Viewer::ReplayHit);
    connect(clusterReplayBtn, &QPushButton::pressed, this, &Viewer::ReplayCluster);

    return page;
}

////////////////////////////////////////////////////////////////
// set up advanced viewer setting

QWidget* Viewer::createAdvancedPage()
{
    QWidget *page = new QWidget(this);
    QFormLayout *form = new QFormLayout(page);
    form -> setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QGroupBox *box = new QGroupBox(tr("GEM Data Viewer Mode"), page);
    QVBoxLayout *v = new QVBoxLayout(box);
    v -> setContentsMargins(8, 8, 8, 8);
    
    QRadioButton* cb_offline = new QRadioButton("Offline Mode", box);
    QRadioButton* cb_online = new QRadioButton("Online Mode", box);
    QButtonGroup* group = new QButtonGroup(box);
    group -> addButton(cb_offline);
    group -> addButton(cb_online);
    group -> setExclusive(true);
    cb_offline -> setChecked(true);

    v -> addWidget(cb_offline);
    v -> addWidget(cb_online);

    form -> addRow(box);

    // signal-slot

    return page;
}

////////////////////////////////////////////////////////////////
// setup the control interface

QWidget* Viewer::createSystemLogPanel()
{
    QGroupBox *box = new QGroupBox(tr("System Log"), this);
    QVBoxLayout *v = new QVBoxLayout(box);
    v -> setContentsMargins(8, 8, 8, 8);

    m_logEdit = new QPlainTextEdit(box);
    m_logEdit -> setReadOnly(true);
    m_logEdit -> setWordWrapMode(QTextOption::NoWrap);
    m_logEdit -> setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_logEdit -> setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logEdit -> setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logEdit -> setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_logEdit -> setWordWrapMode(QTextOption::WrapAnywhere);

    v -> addWidget(m_logEdit);

    m_logEdit -> appendPlainText("[info] GEM Data Viewer started.");

    return box;
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

    m_logEdit -> appendPlainText("[info] Openning file: " + s);

    pGEMAnalyzer -> CloseFile();
    pGEMAnalyzer -> SetFile(s.toStdString().c_str());
    pGEMAnalyzer -> Init();
}

////////////////////////////////////////////////////////////////
//  init gem analyzer

void Viewer::InitGEMAnalyzer()
{
    // setup pedestal before initializing analyzer
    SetPedestalInputPath(QString::fromStdString(txt_parser.Value<std::string>("GEM Pedestal")));
    SetCommonModeInputPath(QString::fromStdString(txt_parser.Value<std::string>("GEM Common Mode")));

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
    std::string ss("[info] total number of APVs in event #");
    ss = ss + std::to_string(m_eventSpin->value()) + " : " + std::to_string(mData.size());
    m_logEdit -> appendPlainText(ss.c_str());

    std::vector<std::vector<std::vector<int>>> vH(nTab);
    std::vector<std::vector<APVAddress>> vAddr(nTab);
#ifdef SHOW_APV_BY_MPD
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
    for(auto &i: mData) {
        MPDAddress mpd_addr(i.first.crate_id, i.first.mpd_id);
        int index = find_index(mpd_addr);
        if(index >= 0 && index < nTab) {
            vH[index].push_back(i.second);
            vAddr[index].push_back(i.first);
        }
    }
#else
    // dispatch by APV counts
    int apv_count = 0;
    int napvs_per_tab = APVS_PER_TAB_X * APVS_PER_TAB_Y;
    for(auto &i: mData) {
        int index = apv_count / napvs_per_tab;
        if(index >= nTab) {
            m_logEdit -> appendPlainText("[error]: number of APVs in data exceeded the maximum allowed in mapping file.");
            m_logEdit -> appendPlainText("[error]:        ------- mapping file is wrong.");
            continue;
        }
        vH[index].push_back(i.second);
        vAddr[index].push_back(i.first);
        apv_count++;
    }
#endif

    // draw tab main canvas
    for(int i=0;i<nTab;i++) 
    {
        vTabCanvas[i] -> Clear();
        vTabCanvas[i] -> DrawCanvas(vH[i], vAddr[i], APVS_PER_TAB_X, APVS_PER_TAB_Y);
        vTabCanvas[i] -> Refresh();
    }

    // draw right-side canvas
    std::vector<std::vector<int>> temp;
    std::vector<APVAddress> temp_addr;
    temp.push_back(mData.begin()->second);
    temp_addr.push_back(mData.begin()->first);
    //pRightCanvas->DrawCanvas(temp, temp_addr, 1, 1);
}


////////////////////////////////////////////////////////////////
// draw extracted gem online hits (fired strips after zero
// suppression for each GEM Chamber)

void Viewer::DrawGEMOnlineHits(int num)
{
    if(reload_pedestal_for_online) {
        QString s = QString("[info] loading pedestal for online analysis from : ")
                    + QString::fromStdString(fPedestalInputPath) + QString(" and ")
                    + QString::fromStdString(fCommonModeInputPath);
        m_logEdit -> appendPlainText(s);
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

        // fill ghost apv data
        GEMAPV *ghost_apv = pGEMReplay -> GetGEMSystem() -> GetGhostAPV(i.first);
        if( ghost_apv != nullptr) {
            ghost_apv -> FillRawDataMPD(i.second, event_data_flag.at(i.first));
            ghost_apv -> ZeroSuppression();
            ghost_apv -> CollectZeroSupHits();
        }
    }

    // organize online hits by layer
    auto & layerID = apv_strip_mapping::Mapping::Instance() -> GetLayerIDVec();

    // online hits contents
    // (x_hits, y_hits)[layer][chamber] -- default 4 chambers per layer
    using Hits2D = std::pair<std::vector<int>, std::vector<int>>;
    std::vector<std::vector<Hits2D>> online_hits(
            layerID.size(), std::vector<Hits2D>(4)
            );
    //std::pair<std::vector<int>, std::vector<int>> online_hits[layerID.size()][4];

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
            if(xPlane) {
                // x plane needs to go to local GEM coord
                // y plane is already in local GEM coord, b/c for SBS arrangement, y is the shorter side
                hit_pos -= APV_STRIP_SIZE * GEMPos * nAPV;
            }
            if(hit_pos >= nAPV * APV_STRIP_SIZE) {
                std::cout<<"Warning: strip no: "<<hit_pos<<", exceeds vector size: "
                    <<nAPV * APV_STRIP_SIZE<<", probably due to special strip treatments."<<std::endl;
                continue;
            }
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

        Prepare2DGeoHits(i);
        GEMPlane *pln_x = i -> GetPlane(GEMPlane::Plane_X);
        GEMPlane *pln_y = i -> GetPlane(GEMPlane::Plane_Y);
        std::vector<int> x_online_hits, y_online_hits;

        if(pln_x != nullptr) {
            const std::vector<StripHit> & x_hits = pln_x -> GetStripHits();
            int x_apvs = pln_x -> GetCapacity();
            x_online_hits = get_histo(x_hits, x_apvs, true, chamber_pos);
            // clear strip hits for next event
            pln_x -> ClearStripHits();

        }

        if(pln_y != nullptr){
            const std::vector<StripHit> & y_hits = pln_y -> GetStripHits();
            int y_apvs = pln_y -> GetCapacity();
            y_online_hits = get_histo(y_hits, y_apvs, false, chamber_pos);
            // clear strip hits for next event
            pln_y -> ClearStripHits();
        }

        int index = get_vector_index(layer_id);

        // layer id not found in mapping file
        if(index < 0)
            continue;

        online_hits[index][chamber_pos] = std::pair<std::vector<int>,
            std::vector<int>>(x_online_hits, y_online_hits);
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

    // emit a signal when done
    emit onlineHitsDrawn(detector_2d_geo_hits);
}


////////////////////////////////////////////////////////////////
// save current event

void Viewer::SaveCurrentEvent()
{
    std::string file_name = "./Rootfiles/event_" + 
        std::to_string(current_event_number) + ".txt";

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

    m_logEdit -> appendPlainText("[info] event saved to: " + QString::fromStdString(file_name));
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
    QString filename =  QFileDialog::getOpenFileName(
            this, tr("Open EVIO file"), QString(),
            tr("EVIO files (*.evio*);;All files (*)"));
    if(!filename.isEmpty()) {
        int idx = m_fileCombo -> findText(filename);
        if(idx < 0) {
            m_fileCombo -> insertItem(0, filename);
            idx = 0;
        }
        m_fileCombo -> setCurrentIndex(idx);
    }

    fFile = filename.toStdString();

    if(fFile.size() <= 0) 
        return;

    // reset event counter to 0
    m_eventSpin -> setValue(0);
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

    m_pedOut -> setText(fPedestalOutputPath.c_str());
    m_cmOut -> setText(fCommonModeOutputPath.c_str());
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
// reload pedestal and common mode

void Viewer::ReloadPedestal()
{
    // reset online pedestal
    reload_pedestal_for_online = true;

    if(fPedestalInputPath.size() <= 0)
        fPedestalInputPath = "database/gem_ped.dat";

    if(fCommonModeInputPath.size() <= 0)
        fCommonModeInputPath = "database/CommonModeRange.txt";
}

////////////////////////////////////////////////////////////////
// set root file output path

void Viewer::SetRootFileOutputPath(const QString &s)
{
    fRootFileSavePath = s.toStdString();
}

////////////////////////////////////////////////////////////////
// set input file split max [min, max]

void Viewer::SetFileSplitMax(const int &s)
{
    fFileSplitEnd = s;
    m_logEdit -> appendPlainText("[info] setting maximum file split: " + QString::number(fFileSplitEnd));
}

////////////////////////////////////////////////////////////////
// set input file split min [min, max]

void Viewer::SetFileSplitMin(const int &s)
{
    if( s < 0) {
        m_logEdit -> appendPlainText("[info] Replay Evio File Split start Number Invalid. Start Number not changed.");
        return;
    }
    fFileSplitStart = s;
    m_logEdit -> appendPlainText("[info] setting minimum file split: " + QString::number(fFileSplitStart));
}

////////////////////////////////////////////////////////////////
// set pedestal max events

void Viewer::SetPedestalMaxEvents(const int & s)
{
    if (s <= 0) {
        m_logEdit -> appendPlainText("[Warning] max events for generating pedestal is invalid, using default 5000.");
        return;
    }
    fPedestalMaxEvents = s;
    m_logEdit -> appendPlainText("[info] max events for generating pedestals: " 
            + QString::number(fPedestalMaxEvents));
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

    m_logEdit -> appendPlainText("[info] generating pedestal/commonMode for file:");
    m_logEdit -> appendPlainText(fFile.c_str());
    m_logEdit -> appendPlainText("[info] this might take a while...");
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

    m_logEdit -> appendPlainText("[info] Generating pedestal/commonMode for file : ");
    m_logEdit -> appendPlainText(fFile.c_str());
    m_logEdit -> appendPlainText("[info] this might take a while...");
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

    m_logEdit -> appendPlainText("[info] Replaying for file: ");
    m_logEdit -> appendPlainText(fFile.c_str());
    m_logEdit -> appendPlainText("[info] this might take a while...");
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

    m_logEdit -> appendPlainText("[info] Replaying for file: ");
    m_logEdit -> appendPlainText(fFile.c_str());
    m_logEdit -> appendPlainText("[info] this might take a while...");
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
}

void Viewer::OpenOnlineAnalysisInterface()
{
    winOnlineInterface = new OnlineAnalysisInterface();
    winOnlineInterface -> show();
}

void Viewer::Prepare2DGeoHits(GEMDetector *det)
{
    // for online 2d hits drawing, match clusters according to ADC, only keep coords (x, y) for simplicity
    GEMCluster *cluster_method = pGEMReplay -> GetGEMSystem() -> GetClusterMethod();

    // extract new hits for detector plane
    auto get_plane_hits = [&](GEMPlane *pln) -> std::vector<std::pair<double, double>>
    {
        std::vector<std::pair<double, double>> res;
        if(pln == nullptr) {
            return res;
        }
        pln -> FormClusters(cluster_method);
        auto strip_clusters = pln -> GetStripClusters();

        for(auto &i: strip_clusters) {
            res.emplace_back(i.peak_charge, i.position);
        }

        // clear strip clusters -- not needed for online anymore
        pln -> ClearStripClusters();

        // sort according to ADC, bigger ones appear first
        std::sort(res.begin(), res.end(),
                [](const auto &a, const auto &b)  {
                return a.first > b.first;
                });

        return res;
    };

    QVector<QPointF> res;
    if(det == nullptr) {
        std::cout<<"detector is nullptr."<<std::endl;
    }

    auto x_hits = get_plane_hits(det -> GetPlane(GEMPlane::Plane_X));
    auto y_hits = get_plane_hits(det -> GetPlane(GEMPlane::Plane_Y));

    // match according to ADC
    size_t N = x_hits.size() > y_hits.size() ? y_hits.size() : x_hits.size();

    for(size_t i=0; i<N; i++) {
        // .second is position; .first is adc
        double x = x_hits[i].second, y = y_hits[i].second;
        res.emplace_back(x, y);
    }

    int id = det -> GetDetID();

    detector_2d_geo_hits[id] = res;
}
