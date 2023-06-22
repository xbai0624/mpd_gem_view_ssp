#include "Viewer.h"
#include "AbstractDetector.h"
#include "Detector2DItem.h"
#include "Detector2DView.h"
#include "Tracking.h"
#include "TrackingDataHandler.h"
#include "TrackingUtility.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QSpinBox>
#include <TRandom.h>
#include <chrono>

namespace tracking_dev {

//#define USE_SIM_DATA

Viewer::Viewer(QWidget *parent) : QWidget(parent)
{
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
}

void Viewer::InitToyDetectorSetup()
{
    tracking = new Tracking();

    for(int i=0; i<NDET_SIM; i++)
    {
        point_t dimension(60, 60, 0.1);
        point_t origin(0, 0, (double)i*30);

        fDet[i] = new AbstractDetector();
        fDet[i] -> SetOrigin(origin);
        fDet[i] -> SetDimension(dimension);

        tracking -> AddDetector(i, fDet[i]);
    }

    tracking -> CompleteSetup();
}

void Viewer::InitGui()
{
    fDet2DView = new Detector2DView(this);

#ifdef USE_SIM_DATA
    NDetector_Implemented = NDET_SIM;
#else
    NDetector_Implemented = tracking_data_handler -> GetNumberofDetectors();
#endif

    for(int i=0; i<NDetector_Implemented; i++)
    {
        fDet2DItem[i] = new Detector2DItem();

#ifdef USE_SIM_DATA
        fDet2DItem[i] -> PassDetectorHandle(fDet[i]);
#else
        fDet[i] = tracking_data_handler->GetDetector(i);
        fDet2DItem[i] -> PassDetectorHandle(fDet[i]);
#endif

        std::string title = std::string("layer ") + std::to_string(i)
            + std::string(", z = ") + std::to_string((int)fDet[i]->GetZPosition())
            + std::string(" mm");
        fDet2DItem[i] -> SetTitle(title.c_str());

        fDet2DView -> AddDetector(fDet2DItem[i]);
    }
    fDet2DView -> InitView();

    btn_next = new QSpinBox(this);
    btn_next -> setRange(0, 9999999);
    btn_50K = new QPushButton("Replay 50K", this);
    btn_open_file = new QPushButton("Open File", this);
    label_counter = new QLabel("Event Number: 0", this);
    label_file = new QLineEdit("../data/hallc_fadc_ssp_4818.evio.1", this);

    global_layout = new QVBoxLayout(this);
    global_layout -> addWidget(fDet2DView);

    QHBoxLayout *_tmplayout = new QHBoxLayout();
    _tmplayout -> addWidget(btn_open_file);
    _tmplayout -> addWidget(label_file);
    _tmplayout -> addWidget(label_counter);
    _tmplayout -> addWidget(btn_50K);
    _tmplayout -> addWidget(btn_next);
    global_layout -> addLayout(_tmplayout);

    connect(btn_open_file, SIGNAL(clicked()), this, SLOT(OpenFile()));
    connect(label_file, SIGNAL(textChanged(const QString &)), this, SLOT(ProcessNewFile(const QString &)));
    connect(btn_next, SIGNAL(valueChanged(int)), this, SLOT(DrawEvent(int)));
    connect(btn_50K, SIGNAL(clicked()), this, SLOT(Replay50K()));
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
        return;
    }
 
#ifdef USE_SIM_DATA
    ClearPrevEvent();
    GenerateToyTrackEvent();
    AddToyEventBackground();
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
        std::cout<<" : det_"<<i<<" counts = "<<fDet[i] -> Get2DHitCounts();
    std::cout<<std::endl;

    fEventNumber++;
    label_counter -> setText((std::string("Event Number: ")+std::to_string(fEventNumber)).c_str());

    auto t1 = Time::now();
    fsec fs = t1 - t0;
    std::cout << fs.count() << " s\n";

    fDet2DView -> Refresh();
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

    // get offset
    const std::vector<int> & layer_index = tracking -> GetBestTrackLayerIndex();
    const std::vector<int> & hit_index = tracking -> GetBestTrackHitIndex();
    for(unsigned int i=0; i<layer_index.size(); i++)
    {
        int layer = layer_index[i];
        int hit_id = hit_index[i];

        point_t p = fDet[layer] -> Get2DHit(hit_id);
        hist_m.histo_1d<float>(Form("h_max_timebin_x_plane_gem%d", layer)) -> Fill(p.x_max_timebin);
        hist_m.histo_1d<float>(Form("h_max_timebin_y_plane_gem%d", layer)) -> Fill(p.y_max_timebin);
        hist_m.histo_1d<float>(Form("h_cluster_size_x_plane_gem%d", layer)) -> Fill(p.x_size);
        hist_m.histo_1d<float>(Form("h_cluster_size_y_plane_gem%d", layer)) -> Fill(p.y_size);
        hist_m.histo_1d<float>(Form("h_cluster_adc_x_plane_gem%d", layer)) -> Fill(p.x_peak);
        hist_m.histo_1d<float>(Form("h_cluster_adc_y_plane_gem%d", layer)) -> Fill(p.y_peak);

        fDet[layer] -> AddRealHits(p);
    }

    for(int i=0; i<NDetector_Implemented; i++)
    {
        point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, fDet[i]->GetZPosition());
        fDet[i] -> AddFittedHits(p);
    }

    int n_good_track_candidates = tracking -> GetNGoodTrackCandidates();

    if(n_good_track_candidates > 0)
        hist_m.histo_1d<float>("h_ntrack_candidates") -> Fill(n_good_track_candidates);

    FillEventHistos();
}

bool Viewer::ProcessRawGEMResult()
{
    bool res = true;

    GEMSystem *gem_sys = tracking_data_handler -> GetGEMSystem();
    std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();
    for(auto &det: detectors)
    {
        int layer = det -> GetLayerID();
        GEMPlane *pln_x = det -> GetPlane(GEMPlane::Plane_X);
        GEMPlane *pln_y = det -> GetPlane(GEMPlane::Plane_Y);

        std::vector<StripHit> &x_hits = pln_x -> GetStripHits();
        std::vector<StripHit> &y_hits = pln_y -> GetStripHits();
        int xs = (int)x_hits.size(), ys = (int)y_hits.size();

        hist_m.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 0)) -> Fill(layer, xs);
        hist_m.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 1)) -> Fill(layer, ys);

        hist_m.histo_2d<float>(Form("h_raw_occupancy_plane%d", 0)) -> Fill(layer, xs/256.);
        hist_m.histo_2d<float>(Form("h_raw_occupancy_plane%d", 1)) -> Fill(layer, ys/256.);
    }

    std::vector<int> v_nhits;
    for(auto &i: detectors)
    {
        size_t s = (i -> GetHits()).size();
        v_nhits.push_back(s);
    }
    for(auto &i: v_nhits) {
        if(res)
            res = (i==1);
    }

    return res;
}

void Viewer::Replay50K()
{
    std::cout<<"processing 50K replay..."<<std::endl;
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::duration<float> fsec;
    auto t0 = Time::now();
    auto t1 = t0;

    int event_counter = 0;

#ifndef USE_SIM_DATA
    tracking_data_handler -> SetReplayMode(true);
#endif

    while(event_counter++ < 50000)
    {
	if(event_counter % 1000 == 0) {
            t1 = Time::now();
	    fsec fs_ = t1 - t0;

	    std::cout<<"\r"<<event_counter<<" time used: "<<fs_.count() <<" s"<<std::flush;
	    t0 = t1;
	}

#ifdef USE_SIM_DATA
        ClearPrevEvent();
        GenerateToyTrackEvent();
        AddToyEventBackground();
#else
        tracking_data_handler -> GetCurrentEvent();
#endif

        tracking -> FindTracks();
        ProcessTrackingResult();
        [[maybe_unused]]bool t = ProcessRawGEMResult();
        //if(t) break;
    }

    std::cout<<std::endl<<"50K finished. Total time used: ";
    t1 = Time::now();
    fsec fs = t1 - t0;
    std::cout << fs.count() << " s\n";

    fDet2DView -> Refresh();

    for(int i=0; i<NDetector_Implemented; i++) {
        for(int xbins = 1; xbins < 120; xbins++)
        {
            for(int ybins = 1; ybins < 120; ybins++) {
                float did = hist_m.histo_2d<float>((Form("h_didhit_xy_gem%d", i))) -> GetBinContent(xbins, ybins);
                float should = hist_m.histo_2d<float>((Form("h_shouldhit_xy_gem%d", i))) -> GetBinContent(xbins, ybins);

                float eff = 0;
                if(should >0) eff = (did / should < 1) ? did / should : 1.;
                hist_m.histo_2d<float>(Form("h_2defficiency_xy_gem%d", i)) -> SetBinContent(xbins, ybins, eff);
            }
        }
    }

    hist_m.save("Rootfiles/tracking_result.root");
}

void Viewer::FillEventHistos()
{
    for(int i=0; i<NDetector_Implemented; i++)
    {
        const std::vector<point_t> & real_hits = fDet[i] -> GetRealHits();
        const std::vector<point_t> & fitted_hits = fDet[i] -> GetFittedHits();

        for(unsigned int hitid=0; hitid<real_hits.size(); hitid++){
            hist_m.histo_1d<float>(Form("h_x_offset_gem%d", i)) -> Fill(real_hits[hitid].x - fitted_hits[hitid].x);
            hist_m.histo_1d<float>(Form("h_y_offset_gem%d", i)) -> Fill(real_hits[hitid].y - fitted_hits[hitid].y);

            hist_m.histo_2d<float>(Form("h_didhit_xy_gem%d", i)) -> Fill(real_hits[hitid].x, real_hits[hitid].y);
        }

        for(auto &h: fitted_hits) {
            hist_m.histo_2d<float>(Form("h_shouldhit_xy_gem%d", i)) -> Fill(h.x, h.y);
        }
    }
}

void Viewer::ShowGridHitStat()
{
    for(int i=0; i<NDetector_Implemented; i++) {
        std::cout<<"detector : "<<i<<std::endl;
        fDet[i] -> ShowGridHitStat();
    }
}

};
