#include "Viewer.h"
#include "VirtualDetector.h"
#include "Detector2DHitItem.h"
#include "Detector2DHitView.h"
#include "IsometricView.h"
#include "Tracking.h"
#include "TrackingDataHandler.h"
#include "TrackingUtility.h"
#include "TrackingConfigWidget.h"
#include "TrackingResultPanel.h"
#include "Cuts.h"
#include "GEMStruct.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QSpinBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QGroupBox>
#include <QStatusBar>
#include <QSplitter>
#include <TRandom.h>
#include <chrono>

namespace tracking_dev {

//#define USE_SIM_DATA

Viewer::Viewer(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("Tracking Viewer"));

    // root
    gen = new TRandom(0);

#ifdef USE_SIM_DATA
    InitToyDetectorSetup();
#else
    tracking_data_handler = new TrackingDataHandler();
    tracking_data_handler -> Init();
    tracking_data_handler -> SetupDetector();

    tracking = tracking_data_handler -> GetTrackingHandle();
#endif

    InitGui();

    hist_m.init();
}

Viewer::~Viewer()
{
    if(m_replay50KThread.joinable())
        m_replay50KThread.join();
}

void Viewer::InitToyDetectorSetup()
{
    tracking = new Tracking();

    double z_origin[NDET_SIM] = {0, 100, 1020, 1120};

    // detector grids
    double xw = Cuts::Instance().__get("grid width").arr<double>()[0];
    double yw = Cuts::Instance().__get("grid width").arr<double>()[1];
    double s = Cuts::Instance().__get("grid shift").val<double>();

    for(int i=0; i<NDET_SIM; i++)
    {
        point_t dimension(100, 100, 0.1);
        //point_t origin(0, 0, (double)i*30);
        point_t origin(0, 0, z_origin[i]);

        fDet[i] = new VirtualDetector();
        fDet[i] -> SetOrigin(origin);
        fDet[i] -> SetGridWidth(xw, yw);
        fDet[i] -> SetGridShift(s);
        fDet[i] -> SetDimension(dimension);

        layer_ids.push_back(i);
        tracking -> AddLayer(i, fDet[i]);
    }

    tracking -> CompleteSetup();

    LoadFermiData();
}

void Viewer::InitGui()
{
    InitDetectorViews();                    // 2D items + iso view
    ApplyTheme();                           // stylesheet
    createMenuBar();                        // File / Edit / Parameters
    setCentralWidget(createCentralArea());  // toolbar row + splitters
    createStatusBar();                      // Event / Tracks / chi2 / timing
    InitConnections();                      // all signal wiring
}

////////////////////////////////////////////////////////////////
// 2D detector items + 3D isometric view

void Viewer::InitDetectorViews()
{
    fDet2DView = new Detector2DHitView(this);

#ifdef USE_SIM_DATA
    NDetector_Implemented = NDET_SIM;
#else
    NDetector_Implemented = tracking_data_handler -> GetNumberofLayers();
    layer_ids = tracking_data_handler -> GetLayerIDs();
#endif

    for(int i=0; i<NDetector_Implemented; i++)
    {
        int layer_id = layer_ids[i];
        fDet2DItem[layer_id] = new Detector2DHitItem();

#ifdef USE_SIM_DATA
        fDet2DItem[layer_id] -> PassDetectorHandle(fDet[layer_id]);
#else
        fDet[layer_id] = tracking_data_handler->GetLayer(layer_id);
        fDet2DItem[layer_id] -> PassDetectorHandle(fDet[layer_id]);
#endif

        std::string title = std::string("Layer ") + std::to_string(layer_id)
            + std::string(", z = ") + std::to_string((int)fDet[layer_id]->GetZPosition())
            + std::string(" mm");
        fDet2DItem[layer_id] -> SetTitle(title.c_str());

        fDet2DView -> AddDetector(fDet2DItem[layer_id]);
    }
    fDet2DView -> InitView();

    // Build the sibling 3D isometric view from the SAME detector list,
    // so editing gem_mapping.txt updates both views with no extra wiring.
    fIsoView = new IsometricView(this);
    for(int i = 0; i < NDetector_Implemented; i++)
        fIsoView -> AddLayer(fDet[layer_ids[i]]);
    fIsoView -> InitView();
}

////////////////////////////////////////////////////////////////
// window stylesheet

void Viewer::ApplyTheme()
{
    // theme matching gui/src/Viewer.cpp (buttons/inputs stay native)
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
            }
            )");
}

////////////////////////////////////////////////////////////////
// menu bar

void Viewer::createMenuBar()
{
    // ---- menu bar: File ----
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    QAction *openAction = fileMenu->addAction(tr("open"));

     // ---- menu bar: Edit ----
    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    QAction *undoAction = editMenu->addAction(tr("undo"));

    // ---- menu bar: Settings -> Tracking Configuration... (pop-up) ----
    QMenu *settingsMenu = menuBar()->addMenu(tr("Parameters"));
    QAction *cfgAction = settingsMenu->addAction(tr("Tracking Configuration..."));
    connect(cfgAction, &QAction::triggered, this, &Viewer::OpenSettings);
}

////////////////////////////////////////////////////////////////
// top toolbar: file selector + event navigation + replay

QWidget* Viewer::createTopToolbar()
{
    btn_next = new QSpinBox(this);
    btn_next -> setRange(0, 9999999);
    btn_prev = new QPushButton(QString::fromUtf8("◀ Prev"), this);
    btn_next_step = new QPushButton(QString::fromUtf8("Next ▶"), this);
    btn_50K = new QPushButton("Replay 50K", this);
    btn_open_file = new QPushButton("Open File", this);
    //label_file = new QLineEdit("../data/hallc_fadc_ssp_4818.evio.1", this);
    label_file = new QLineEdit("../data/fermilab_run_1018.evio.1", this);

    // ---- top toolbar as a plain widget row (matches gui/src/Viewer.cpp:
    //      a QHBoxLayout of native widgets, not a QToolBar) ----
    QWidget *topToolbar = new QWidget(this);
    QHBoxLayout *tbar = new QHBoxLayout(topToolbar);
    label_file -> setMinimumWidth(300);
    tbar -> addWidget(new QLabel(tr("File:"), topToolbar));
    tbar -> addWidget(label_file);
    tbar -> addWidget(btn_open_file);
    tbar -> addSpacing(16);
    tbar -> addWidget(new QLabel(tr("Event:"), topToolbar));
    tbar -> addWidget(btn_prev);
    tbar -> addWidget(btn_next);
    tbar -> addWidget(btn_next_step);
    tbar -> addStretch(1);
    tbar -> addWidget(btn_50K);

    return topToolbar;
}

////////////////////////////////////////////////////////////////
// central area: toolbar row + GEM Hits pane + Tracking Results pane

QWidget* Viewer::createCentralArea()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *vMain = new QVBoxLayout(central);
    vMain -> setContentsMargins(8, 8, 8, 8);
    vMain -> setSpacing(8);
    vMain -> addWidget(createTopToolbar());

    QGroupBox *plotBox = new QGroupBox(tr("GEM Hits"), central);
    QVBoxLayout *plotLayout = new QVBoxLayout(plotBox);
    plotLayout -> addWidget(fDet2DView);

    QGroupBox *resultBox = new QGroupBox(tr("Tracking Results"), central);
    QVBoxLayout *resultLayout = new QVBoxLayout(resultBox);

    // left = main Det2D plots, right = result-histogram panel
    result_panel = new TrackingResultPanel(resultBox);
    // wrap the 3D isometric view + result-histogram panel in a vertical
    // splitter so the user can resize them.
    QSplitter *rightSplit = new QSplitter(Qt::Vertical, resultBox);
    rightSplit -> addWidget(fIsoView);
    rightSplit -> addWidget(result_panel);
    rightSplit -> setStretchFactor(0, 2);
    rightSplit -> setStretchFactor(1, 1);
    resultLayout -> addWidget(rightSplit);
    // pre-create the category sections (empty placeholders) so the panel
    // shows the accordion headers at startup instead of a blank box;
    // Replay 50K (UpdateResultHistos) clears and refills them with data.
    result_panel -> AddSection("Tracking Histos");
#ifdef USE_SIM_DATA
    for(int i = 0; i < NDetector_Implemented; ++i)
        result_panel -> AddSection(Form("GEM %d", i));
#else
    // label placeholders by module_id so they match the names UpdateResultHistos
    // uses after Replay 50K (no jump from "GEM 0..N-1" to "GEM <module_id>")
    for(int module_id : tracking_data_handler -> GetDetectorModuleIDs())
        result_panel -> AddSection(Form("GEM Module %d", module_id));
#endif

    QSplitter *split = new QSplitter(Qt::Horizontal, central);
    split -> addWidget(plotBox);
    split -> addWidget(resultBox);
    split -> setStretchFactor(0, 3);
    split -> setStretchFactor(1, 1);
    vMain -> addWidget(split, 1);

    return central;
}

////////////////////////////////////////////////////////////////
// status bar readout

void Viewer::createStatusBar()
{
    m_statEvent  = new QLabel(tr(" Event: 0 "), this);
    m_statTracks = new QLabel(tr(" Tracks found: 0 (out of 0 good candidates)"), this);
    m_statChi2   = new QLabel(tr(" best chi2/ndf: - "), this);
    m_statTiming = new QLabel(tr("idle"), this);
    m_statTiming -> setMinimumWidth(240);   // room for the progress text
    statusBar() -> addWidget(m_statEvent);
    statusBar() -> addWidget(m_statTracks);
    statusBar() -> addWidget(m_statChi2);
    // timing/progress on the right (permanent so a transient showMessage
    // never hides it)
    statusBar() -> addPermanentWidget(m_statTiming);
}

////////////////////////////////////////////////////////////////
// signal wiring

void Viewer::InitConnections()
{
    connect(btn_open_file, SIGNAL(clicked()), this, SLOT(OpenFile()));
    connect(label_file, SIGNAL(textChanged(const QString &)), this, SLOT(ProcessNewFile(const QString &)));
    connect(btn_next, SIGNAL(valueChanged(int)), this, SLOT(DrawEvent(int)));
    connect(btn_50K, SIGNAL(clicked()), this, SLOT(Replay50K()));
    connect(this, &Viewer::ReplayProgress,
            this, &Viewer::UpdateReplayProgress, Qt::QueuedConnection);
    connect(this, &Viewer::ReplayFinished,
            this, &Viewer::FinishReplay50K, Qt::QueuedConnection);

    // Prev/Next just nudge the spinbox -> reuses the DrawEvent path
    connect(btn_prev, &QPushButton::clicked, this,
            [this]{ btn_next->setValue(btn_next->value() - 1); });
    connect(btn_next_step, &QPushButton::clicked, this,
            [this]{ btn_next->setValue(btn_next->value() + 1); });
}

////////////////////////////////////////////////////////////////
// Settings menu -> tracking configuration pop-up window

void Viewer::OpenSettings()
{
    if(!settings_window) {
        settings_window = new QWidget(this, Qt::Window);
        settings_window -> setWindowTitle(tr("Tracking Settings"));
        settings_window -> resize(440, 640);

        QVBoxLayout *lay = new QVBoxLayout(settings_window);
        lay -> setContentsMargins(0, 0, 0, 0);

        config_panel = new TrackingConfigWidget(
                QString::fromStdString(Cuts::Instance().GetConfigPath()),
                settings_window);
        QScrollArea *sc = new QScrollArea(settings_window);
        sc -> setWidget(config_panel);
        sc -> setWidgetResizable(true);
        lay -> addWidget(sc);

        // applying the config re-applies it to the running tracking setup;
        // takes effect on the next event. Catch a bad config edit so a
        // typo (missing key, dropped value) shows a modal dialog instead
        // of terminating the app -- Cuts::Reload is atomic so the live
        // singleton stays at the previous-good state when this throws.
#ifndef USE_SIM_DATA
        connect(config_panel, &TrackingConfigWidget::applied, this, [this]{
            if(!tracking_data_handler) return;
            try {
                tracking_data_handler -> ReapplyConfig();
            }
            catch(const std::exception &e) {
                QMessageBox::critical(this, tr("Tracking Configuration"),
                        tr("Cannot apply config:\n\n%1\n\n"
                           "Previous configuration is still in effect.")
                        .arg(QString::fromStdString(e.what())));
            }
        });
#endif
    }

    settings_window -> show();
    settings_window -> raise();
    settings_window -> activateWindow();
}

void Viewer::LoadFermiData()
{
    std::cout<<"Loading Fermilab data."<<std::endl;
    const char* path = "./tracking_dev/fermi_tracks.txt";

    std::fstream f(path, std::fstream::in);
    std::string line;

    double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4; 
    while(f >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> x3 >> y3 >> z3 >> x4 >> y4 >> z4)
    {
        point_t p1(x1, y1, z1);
        point_t p2(x2, y2, z2);
        point_t p3(x3, y3, z3);
        point_t p4(x4, y4, z4);

        std::vector<point_t> track;
        track.push_back(p1);
        track.push_back(p2);
        track.push_back(p3);
        track.push_back(p4);

        total_tracks.push_back(track);
    }
}

void Viewer::OpenFile()
{
    QString filename = QFileDialog::getOpenFileName(
            this,
            "Open Document",
            //QDir::currentPath(),
            "/home/daq/coda/data/",
            "All files (*.*) ;; evio files (*.evio)");

    evio_file = filename.toStdString();
    label_file -> setText(evio_file.c_str());
}

void Viewer::ProcessNewFile(const QString &_s)
{
    std::string ss = _s.toStdString();
    std::cout<<"Processing new file: "<<ss<<std::endl;
    tracking_data_handler -> SetEvioFile(ss.c_str());

    // reset event counters
    fEventNumber = 0;
    btn_next -> setValue(0);

    // clear the per-event status cache so Prev/Next on the new file
    // doesn't show stale chi2 / track counts from the previous file.
    m_hist_ntracks.clear();
    m_hist_ncandidates.clear();
    m_hist_chi2.clear();
    m_hist_found.clear();
}

void Viewer::ClearPrevEvent()
{
    for(int i=0; i<NDetector_Implemented; i++)
        fDet[i] -> Reset();
}

void Viewer::GenerateToyTrackEvent()
{
    double xrand = gen -> Uniform(-20., 20.);
    double yrand = gen -> Uniform(-20., 20.);

    static double tanx = 1./10.;
    static double tany = 1./10.;

    double xdir = gen -> Uniform(-tanx, tanx);
    double ydir = gen -> Uniform(-tany, tany);
    static double zdir = 1.;

    point_t dir(xdir, ydir, zdir);
    dir = dir.unit();

    // 0, 2, -1, 3
    //static double x_correct[4] = {0, 0, 0, 0};
    //static double y_correct[4] = {0, 0, 0, 0};
    static double x_correct[4] = {-0.1, 1.3, -2.3, 1.1};
    static double y_correct[4] = {-0.1, 1.3, -2.3, 1.1};

    for(int i=0; i<NDetector_Implemented; i++)
    {
        point_t origin = fDet[i] -> GetOrigin();
        point_t z_axis = fDet[i] -> GetZAxis();

        double r =  origin.mod() / z_axis.dot(dir);

        point_t v = dir * r;

        v.x = v.x + xrand;
        v.y = v.y + yrand;

        // smear by resolution 1mm
        v.x = gen -> Gaus(v.x, 1.) + fXOffset[i] - x_correct[i];
        v.y = gen -> Gaus(v.y, 1.) + fYOffset[i] - y_correct[i];

        fDet[i] -> AddRealHits(v); // for drawing purpose
        fDet[i] -> AddHit(v);
    }
}

void Viewer::GetOneFermiTrack(int i)
{
    if(i >= (int) total_tracks.size()) {
        std::cout<<"finished all fermi tracks..."<<std::endl;
        return;
    }

    auto & track = total_tracks[i];
    for(int j=0; j<NDetector_Implemented; j++)
    {
        fDet[j] -> AddHit(track[j]);
    }
}

void Viewer::AddToyEventBackground()
{
    for(int i=0; i<NDetector_Implemented; i++)
    {
        for(int j=0; j<N_BACKGROUND; j++)
        {
            double xrand = gen -> Uniform(-20., 20.);
            double yrand = gen -> Uniform(-20., 20.);
            // smear by resoution
            xrand = gen -> Gaus(xrand, 1.0) + fXOffset[i];
            yrand = gen -> Gaus(yrand, 1.0) + fYOffset[i];

            point_t temp = fDet[i] -> GetOrigin();

            temp.x = xrand, temp.y = yrand;

            fDet[i] -> AddBackgroundHits(temp); // for drawing purpose
            fDet[i] -> AddHit(temp);
        }
    }
}

void Viewer::DrawEvent(int event_number)
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::duration<float> fsec;
    auto t0 = Time::now();

    int event_diff = event_number - fEventNumber;
    if(event_diff <= 0) {
        fDet2DView -> BringUpPreviousEvent(event_diff);
        fDet2DView -> Refresh();
        fIsoView -> BringUpPreviousEvent(event_diff);
        fIsoView -> Refresh();
        UpdateStatusBar(event_number);   // show that event's cached result
        return;
    }

#ifdef USE_SIM_DATA
    ClearPrevEvent();
    //GenerateToyTrackEvent();
    //AddToyEventBackground();
    GetOneFermiTrack(event_number);
    //ShowGridHitStat();
#else
    tracking_data_handler -> SetOnlineMode(true);
    tracking_data_handler -> ClearPrevEvent();
    tracking_data_handler -> GetCurrentEvent();
    //ShowGridHitStat();
#endif

    tracking -> FindTracks();
    ProcessTrackingResult();

    std::cout<<"event number: "<<event_number<<std::endl;
    for(int i=0; i<NDetector_Implemented; i++)
        std::cout<<" : layer_"<<layer_ids[i]<<" counts = "<<fDet[layer_ids[i]] -> Get2DHitCounts();
    std::cout<<std::endl;

    fEventNumber++;

    // record this event's tracking result so Prev can show it later
    double xt, yt, xp, yp, chi2;
    bool ok = tracking -> GetBestTrack(xt, yt, xp, yp, chi2);
    m_hist_ntracks.push_back(tracking -> GetNTracksFound());
    m_hist_ncandidates.push_back(tracking -> GetNGoodTrackCandidates());
    m_hist_chi2.push_back(ok ? chi2 : 0.0);
    m_hist_found.push_back(ok);

    UpdateStatusBar(fEventNumber);

    auto t1 = Time::now();
    fsec fs = t1 - t0;
    std::cout << fs.count() << " s\n";

    fDet2DView -> Refresh();
    fIsoView -> Refresh();
}

// show the cached tracking result for a given event ordinal (1-based).
void Viewer::UpdateStatusBar(int event_ordinal)
{
    if(!m_statEvent) return;

    m_statEvent -> setText(QString(" Event: %1 ").arg(event_ordinal));

    int idx = event_ordinal - 1;
    if(idx >= 0 && idx < (int)m_hist_found.size()) {
        m_statTracks -> setText(QString(" Tracks found: %1 (out of %2 good candidates) ")
                .arg(m_hist_ntracks[idx])
                .arg(m_hist_ncandidates[idx]));
        m_statChi2   -> setText(m_hist_found[idx]
                ? QString(" best chi2/ndf: %1 ").arg(m_hist_chi2[idx], 0, 'g', 3)
                : QString(" best chi2/ndf: - "));
    } else {
        m_statTracks -> setText(QString(" Tracks found: 0 (out of 0 good candidates) "));
        m_statChi2   -> setText(QString(" best chi2/ndf: - "));
    }

    // Force immediate paint of the labels. On Linux xcb the QLabel paint
    // events get queued behind fDet2DView's repaint, which makes the status
    // bar appear one event behind when stepping. macOS Cocoa coalesces so
    // it wasn't visible there. repaint() is synchronous (no input events
    // processed) so this is safe in the middle of DrawEvent.
    m_statEvent  -> repaint();
    m_statTracks -> repaint();
    m_statChi2   -> repaint();
}

void Viewer::ProcessTrackingResult()
{
    double xt, yt, xp, yp, chi2ndf;
    bool found_track = tracking -> GetBestTrack(xt, yt, xp, yp, chi2ndf);

    if(!found_track) {
        hist_m.histo_1d<float>("h_ntracks_found") -> Fill(0);
        hist_m.histo_1d<float>("h_nhits_on_best_track") -> Fill(0);
        return;
    }

    hist_m.histo_1d<float>("h_ntracks_found") -> Fill(1);
    hist_m.histo_1d<float>("h_nhits_on_best_track") -> Fill(tracking -> GetNHitsonBestTrack());

    // fill best track to histograms
    hist_m.histo_1d<float>("h_xtrack") -> Fill(xt);
    hist_m.histo_1d<float>("h_ytrack") -> Fill(yt);
    hist_m.histo_1d<float>("h_xptrack") -> Fill(xp);
    hist_m.histo_1d<float>("h_yptrack") -> Fill(yp);
    hist_m.histo_1d<float>("h_chi2ndf") -> Fill(chi2ndf);

    point_t pt(xt, yt, 0);
    point_t dir(xp, yp, 1.);

    const std::vector<point_t> &hits_on_best_track = tracking -> GetVHitsOnBestTrack();

    for(const auto &p: hits_on_best_track)
    {
        int module_id = p.module_id;

        hist_m.histo_1d<float>(Form("h_max_timebin_x_plane_gem%d", module_id)) -> Fill(p.x_max_timebin);
        hist_m.histo_1d<float>(Form("h_max_timebin_y_plane_gem%d", module_id)) -> Fill(p.y_max_timebin);
        hist_m.histo_1d<float>(Form("h_cluster_size_x_plane_gem%d", module_id)) -> Fill(p.x_size);
        hist_m.histo_1d<float>(Form("h_cluster_size_y_plane_gem%d", module_id)) -> Fill(p.y_size);
        hist_m.histo_1d<float>(Form("h_cluster_adc_x_plane_gem%d", module_id)) -> Fill(p.x_peak);
        hist_m.histo_1d<float>(Form("h_cluster_adc_y_plane_gem%d", module_id)) -> Fill(p.y_peak);

        VirtualDetector *module_det = tracking_data_handler -> GetDetector(module_id);
        if(module_det != nullptr)
            module_det -> AddRealHits(p);

        auto layer_it = fDet.find(p.layer_id);
        if(layer_it != fDet.end() && layer_it->second != nullptr)
            layer_it->second -> AddRealHits(p);
    }

    const std::vector<int> &det_module_ids = tracking_data_handler -> GetDetectorModuleIDs();
    for(auto &module_id: det_module_ids)
    {
        VirtualDetector *module_det = tracking_data_handler -> GetDetector(module_id);
        if(module_det == nullptr)
            continue;

        point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, module_det->GetZPosition());
        module_det -> AddFittedHits(p);

        int layer_id = module_det -> GetLayerID();
        auto layer_it = fDet.find(layer_id);
        if(layer_it != fDet.end() && layer_it->second != nullptr)
            layer_it->second -> AddFittedHits(p);
    }

    int n_good_track_candidates = tracking -> GetNGoodTrackCandidates();

    if(n_good_track_candidates > 0)
        hist_m.histo_1d<float>("h_ntrack_candidates") -> Fill(n_good_track_candidates);

    FillEventHistos();
}

void Viewer::ProcessRawGEMResult()
{
    GEMSystem *gem_sys = tracking_data_handler -> GetGEMSystem();
    std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();
    for(auto &det: detectors)
    {
        int module_id = det -> GetDetID();
        GEMPlane *pln_x = det -> GetPlane(GEMPlane::Plane_X);
        GEMPlane *pln_y = det -> GetPlane(GEMPlane::Plane_Y);

        int xs = 0;
        int ys = 0;
        double x_occ = 0.;
        double y_occ = 0.;

        if(pln_x != nullptr) {
            const std::vector<StripHit> &x_hits = pln_x -> GetStripHits();
            xs = static_cast<int>(x_hits.size());

            double total = pln_x -> GetCapacity() * APV_STRIP_SIZE;
            if(total > 0)
                x_occ = xs / total;
        }

        if(pln_y != nullptr) {
            const std::vector<StripHit> &y_hits = pln_y -> GetStripHits();
            ys = static_cast<int>(y_hits.size());

            double total = pln_y -> GetCapacity() * APV_STRIP_SIZE;
            if(total > 0)
                y_occ = ys / total;
        }

        // NOTE: the X axis here is module_id. The histo's X-axis range is
        // fixed by config/histo.conf; module_ids outside that range
        // silently land in the overflow bin. PRad-II uses 1..6 -- fine.
        hist_m.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 0)) -> Fill(module_id, xs);
        hist_m.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 1)) -> Fill(module_id, ys);

        hist_m.histo_2d<float>(Form("h_raw_occupancy_plane%d", 0)) -> Fill(module_id, x_occ);
        hist_m.histo_2d<float>(Form("h_raw_occupancy_plane%d", 1)) -> Fill(module_id, y_occ);
    }
}

void Viewer::Replay50K()
{
    if(m_replay50KRunning)
        return;

    if(m_replay50KThread.joinable())
        m_replay50KThread.join();

    std::cout<<"processing 50K replay..."<<std::endl;
    m_replay50KRunning = true;
    SetReplayControlsEnabled(false);
    if(m_statTiming)
        m_statTiming->setText(tr(" replay running... "));

    m_replay50KThread = std::thread(&Viewer::RunReplay50KWorker, this);
}

void Viewer::RunReplay50KWorker()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::duration<float> fsec;
    auto replay_start = Time::now();
    auto t0 = replay_start;

    int event_counter = 0;

#ifndef USE_SIM_DATA
    tracking_data_handler -> SetReplayMode(true);
#endif

    while(event_counter < 50000)
    {
#ifdef USE_SIM_DATA
        ClearPrevEvent();
        GenerateToyTrackEvent();
        AddToyEventBackground();
#else
        tracking_data_handler -> GetCurrentEvent();
#endif

        tracking -> FindTracks();
        ProcessTrackingResult();
        ProcessRawGEMResult();
        //if(t) break;

        event_counter++;

        if(event_counter % 1000 == 0) {
            auto t1 = Time::now();
            fsec fs_ = t1 - t0;

            std::cout<<"\r"<<event_counter<<" events, time used: "<<fs_.count() <<" s"<<std::flush;
            emit ReplayProgress(event_counter, fs_.count());
            t0 = t1;
        }
    }

    std::cout<<std::endl<<"50K finished. Total time used: ";
    auto t1 = Time::now();
    fsec fs = t1 - replay_start;
    std::cout << fs.count() << " s\n";

    FinalizeReplay50KHistos();
    emit ReplayFinished(event_counter, fs.count());
}

void Viewer::UpdateReplayProgress(int events, double seconds_per_1000)
{
    if(m_statTiming) {
        m_statTiming->setText(QString(" %1 events, %2 s / 1000 ")
                .arg(events).arg(seconds_per_1000, 0, 'f', 2));
    }
}

void Viewer::FinishReplay50K(int events, double total_seconds)
{
    if(m_replay50KThread.joinable())
        m_replay50KThread.join();

    if(m_statTiming) {
        m_statTiming->setText(QString(" replay done (%1 events, %2 s total) ")
                .arg(events).arg(total_seconds, 0, 'f', 1));
    }

    m_replay50KRunning = false;
    SetReplayControlsEnabled(true);
    fDet2DView -> Refresh();
    fIsoView -> Refresh();
    UpdateResultHistos();
}

void Viewer::SetReplayControlsEnabled(bool enabled)
{
    if(btn_50K) btn_50K -> setEnabled(enabled);
    if(btn_next) btn_next -> setEnabled(enabled);
    if(btn_prev) btn_prev -> setEnabled(enabled);
    if(btn_next_step) btn_next_step -> setEnabled(enabled);
    if(btn_open_file) btn_open_file -> setEnabled(enabled);
    if(label_file) label_file -> setEnabled(enabled);
}

void Viewer::FinalizeReplay50KHistos()
{
    const std::vector<int> &det_module_ids = tracking_data_handler -> GetDetectorModuleIDs();

    for(auto &module_id: det_module_ids) {
        TH2F *did_h = hist_m.histo_2d<float>(Form("h_didhit_xy_gem%d", module_id));
        TH2F *should_h = hist_m.histo_2d<float>(Form("h_shouldhit_xy_gem%d", module_id));
        TH2F *eff_h = hist_m.histo_2d<float>(Form("h_2defficiency_xy_gem%d", module_id));

        if(did_h == nullptr || should_h == nullptr || eff_h == nullptr)
            continue;

        int nx = did_h -> GetNbinsX();
        int ny = did_h -> GetNbinsY();

        if(should_h -> GetNbinsX() != nx || should_h -> GetNbinsY() != ny ||
           eff_h -> GetNbinsX() != nx || eff_h -> GetNbinsY() != ny) {
            std::cout<<"Viewer::FinalizeReplay50KHistos warning: bin mismatch for GEM "
                     <<module_id<<std::endl;
            continue;
        }

        for(int xbins = 1; xbins <= nx; xbins++)
        {
            for(int ybins = 1; ybins <= ny; ybins++) {
                float did = did_h -> GetBinContent(xbins, ybins);
                float should = should_h -> GetBinContent(xbins, ybins);

                float eff = 0;
                if(should >0) eff = (did / should < 1) ? did / should : 1.;
                eff_h -> SetBinContent(xbins, ybins, eff);
            }
        }
    }

    hist_m.save("Rootfiles/tracking_result.root");
}

////////////////////////////////////////////////////////////////
// copy the selected result histograms from hist_m into the right-side
// TrackingResultPanel (the in-memory histos are identical to those just written
// to Rootfiles/tracking_result.root).

void Viewer::UpdateResultHistos()
{
    if(!result_panel) return;
    result_panel -> Clear();

    // build stats via snprintf into local buffers (NOT Form(): Form reuses a
    // shared ring buffer, so several calls in one expression can alias).
    auto add1d = [&](const char *name)
    {
        TH1F *h = hist_m.histo_1d<float>(name);
        if(!h) return;
        int n = h -> GetNbinsX();
        std::vector<double> y; y.reserve(n);
        for(int b = 1; b <= n; ++b) y.push_back(h -> GetBinContent(b));

        char buf[64];
        std::vector<std::string> stats;
        std::snprintf(buf, sizeof(buf), "Entries %.0f", h -> GetEntries()); stats.push_back(buf);
        std::snprintf(buf, sizeof(buf), "Mean %.3g",    h -> GetMean());    stats.push_back(buf);
        std::snprintf(buf, sizeof(buf), "Std %.3g",     h -> GetStdDev());  stats.push_back(buf);

        result_panel -> AddHisto1D(name, stats, n,
                h -> GetXaxis() -> GetXmin(), h -> GetXaxis() -> GetXmax(), y,
                h -> GetXaxis() -> GetTitle(), h -> GetYaxis() -> GetTitle());
    };

    auto add2d = [&](const char *name)
    {
        TH2F *h = hist_m.histo_2d<float>(name);
        if(!h) return;
        int nx = h -> GetNbinsX(), ny = h -> GetNbinsY();
        std::vector<double> z; z.reserve(nx * ny);
        for(int iy = 1; iy <= ny; ++iy)
            for(int ix = 1; ix <= nx; ++ix)
                z.push_back(h -> GetBinContent(ix, iy));

        char buf[64];
        std::vector<std::string> stats;
        std::snprintf(buf, sizeof(buf), "Entries %.0f", h -> GetEntries()); stats.push_back(buf);

        result_panel -> AddHisto2D(name, stats,
                nx, h -> GetXaxis() -> GetXmin(), h -> GetXaxis() -> GetXmax(),
                ny, h -> GetYaxis() -> GetXmin(), h -> GetYaxis() -> GetXmax(), z,
                h -> GetXaxis() -> GetTitle(), h -> GetYaxis() -> GetTitle());
    };

    result_panel -> AddSection("Tracking");
    add1d("h_chi2ndf");
    add1d("h_ntracks_found");
    add1d("h_ntrack_candidates");
    add1d("h_nhits_on_best_track");
    const std::vector<int> &det_module_ids = tracking_data_handler -> GetDetectorModuleIDs();
    for(auto &module_id: det_module_ids) {
        result_panel -> AddSection(Form("GEM %d", module_id));
        add2d(Form("h_2defficiency_xy_gem%d", module_id));
        add2d(Form("h_shouldhit_xy_gem%d", module_id));
        add2d(Form("h_didhit_xy_gem%d", module_id));
        add1d(Form("h_x_offset_gem%d", module_id));
        add1d(Form("h_y_offset_gem%d", module_id));
    }
}

void Viewer::FillEventHistos()
{
    const std::vector<int> &det_module_ids = tracking_data_handler -> GetDetectorModuleIDs();

    for(auto &module_id: det_module_ids)
    {
        VirtualDetector *det = tracking_data_handler -> GetDetector(module_id);
        if(det == nullptr)
            continue;

        const std::vector<point_t> & real_hits = det -> GetRealHits();
        const std::vector<point_t> & fitted_hits = det -> GetFittedHits();

        size_t n = real_hits.size() < fitted_hits.size() ? real_hits.size() : fitted_hits.size();

        for(size_t hitid=0; hitid<n; hitid++){
            hist_m.histo_1d<float>(Form("h_x_offset_gem%d", module_id)) -> Fill(real_hits[hitid].x - fitted_hits[hitid].x);
            hist_m.histo_1d<float>(Form("h_y_offset_gem%d", module_id)) -> Fill(real_hits[hitid].y - fitted_hits[hitid].y);

            hist_m.histo_2d<float>(Form("h_didhit_xy_gem%d", module_id)) -> Fill(real_hits[hitid].x, real_hits[hitid].y);
        }

        for(auto &h: fitted_hits) {
            hist_m.histo_2d<float>(Form("h_shouldhit_xy_gem%d", module_id)) -> Fill(h.x, h.y);
        }
    }
}

void Viewer::ShowGridHitStat()
{
    for(int i=0; i<NDetector_Implemented; i++) {
        std::cout<<"layer : "<<layer_ids[i]<<std::endl;
        fDet[layer_ids[i]] -> ShowGridHitStat();
    }
}

};
