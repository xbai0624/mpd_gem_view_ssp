#include "ToyModel.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <TRandom.h>
#include "TrackingUtility.h"
#include <TFile.h>
#include <TTree.h>

namespace tracking_dev
{
    ToyModel::ToyModel()
    {
    }

    ToyModel::~ToyModel()
    {
    }

    void ToyModel::Generate()
    {
        static const int NEVENTS = 1e4;
        // toy model setup
        static const int NLayer = 5;
        static const double z[NLayer] = {
            3.0, 13.0, 63.0, 73.0, 103.0};
        static const double dx[] = {
            0.0, 1.0, -0.5, -2.0, -1.0};
        static const double dy[] = {
            0.0, -2.0, 1, 0.5, 1.0};
        static const double dz[] = {
            // this standard alignment is extremely not sensitive to z offset
            // 0.0, 3.0, -2.0, 5.0, 3.0
            0.0, 0.0, 0.0, 0.0, 0.0};
        /*
        static const double ax[] = {
        };
        */
        double resolution = 0.08;

        // save tree variable
        int fNtracks_found;
        int fNhitsOnTrack;
        float fXtrack, fYtrack, fXptrack, fYptrack;
        float fChi2Track;
        std::vector<int> fHitLayer;
        std::vector<float> fHitXlocal, fHitYlocal;
        std::vector<float> fXresid, fYresid;
        TTree *T = new TTree("EvTree", "toy data");
        T->Branch("fNtracks_found", &fNtracks_found, "fNtracks_found/I");
        T->Branch("fNhitsOnTrack", &fNhitsOnTrack, "fNhitsOnTrack/I");
        T->Branch("fXtrack", &fXtrack, "fXtrack/F");
        T->Branch("fYtrack", &fYtrack, "fYtrack/F");
        T->Branch("fXptrack", &fXptrack, "fXptrack/F");
        T->Branch("fYptrack", &fYptrack, "fYptrack/F");
        T->Branch("fChi2Track", &fChi2Track, "fChi2Track/F");
        T->Branch("fHitLayer", &fHitLayer);
        T->Branch("fHitXlocal", &fHitXlocal);
        T->Branch("fHitYlocal", &fHitYlocal);

        // tools from tracking utility
        TrackingUtility *tool = new TrackingUtility();

        TRandom *gen = new TRandom(0);
        auto generate_event = [&](std::vector<point_t> &true_points) -> std::vector<point_t>
        {
            fHitLayer.clear();
            fHitXlocal.clear(), fHitYlocal.clear();
            fXresid.clear(), fYresid.clear();

            double kx = gen->Uniform(-0.15, 0.15);
            double ky = gen->Uniform(-0.15, 0.15);
            double bx = gen->Uniform(-50.0, 50.0);
            double by = gen->Uniform(-50.0, 50.0);
            bx = gen->Gaus(bx, resolution);
            by = gen->Gaus(by, resolution);

            point_t dir(kx, ky, 1.0);
            dir = dir.unit();

            std::vector<point_t> res;
            point_t h0(bx, by, z[0]);
            true_points.push_back(h0);
            res.push_back(h0);
            fHitLayer.push_back(0);
            fHitXlocal.push_back(bx);
            fHitYlocal.push_back(by);
            for (int i = 1; i < NLayer; i++)
            {
                double real_z = z[i] + dz[i];
                point_t hi = tool->projected_point(h0, dir, real_z);
                hi.x = gen->Gaus(hi.x, resolution);
                hi.y = gen->Gaus(hi.y, resolution);

                true_points.push_back(hi);
                hi.x = hi.x + dx[i];
                hi.y = hi.y + dy[i];
                res.push_back(hi);

                fHitLayer.push_back(i);
                fHitXlocal.push_back(hi.x);
                fHitYlocal.push_back(hi.y);
            }
            return res;
        };

        for (int i = 0; i < NEVENTS; i++)
        {
            event_t event, true_event;
            event.hits = generate_event(true_event.hits);
            all_events.push_back(event);
            all_true_events.push_back(true_event);

            // fill tree
            double fXtrack_, fYtrack_, fXptrack_, fYptrack_;
            double fChi2Track_;
            std::vector<double> fHitXlocal_, fHitYlocal_;
            std::vector<double> fXresid_, fYresid_;
            tool->FitLine(event.hits, fXtrack_, fYtrack_, fXptrack_, fYptrack_, fChi2Track_, fXresid_, fYresid_);

            fNtracks_found = 1;
            fNhitsOnTrack = 5;
            fXtrack = fXtrack_;
            fYtrack = fYtrack_;
            fXptrack = fXptrack_;
            fYptrack = fYptrack_;
            fChi2Track = fChi2Track_;

            T->Fill();
        }

        // save events to file
        WriteTextFile(all_events, "alignment/tracks.txt");
        WriteTextFile(all_true_events, "alignment/true_tracks.txt");
        TFile *f = new TFile("alignment/tracks_tree.root", "recreate");
        T->Write();
        f->Close();
    }

    void ToyModel::Load()
    {
        std::fstream f("alignment/tracks.txt", std::fstream::in);
        if (!f.is_open())
        {
            std::cout << "Error: cannot open file alignment/tracks.txt" << std::endl;
            exit(0);
        }

        static const int NLayer = 5;
        double x[NLayer], y[NLayer], z[NLayer];
        while (f >> x[0] >> y[0] >> z[0] >> x[1] >> y[1] >> z[1] >> x[2] >> y[2] >> z[2] >> x[3] >> y[3] >> z[3] >> x[4] >> y[4] >> z[4])
        {
            event_t event;
            for (int i = 0; i < NLayer; i++)
            {
                point_t p(x[i], y[i], z[i]);
                event.hits.push_back(p);
            }
            all_events.push_back(event);
        }

        // WriteTextFile("alignment/tracks_new.txt");
    }

    void ToyModel::WriteTextFile(const std::vector<event_t> & events, const char *path)
    {
        std::fstream f(path, std::fstream::out);
        if (!f.is_open())
        {
            std::cout << "Error: cannot open file: alignment/tracks.txt" << std::endl;
            exit(0);
        }

        for (auto &i : events)
        {
            for (auto &hit : i.hits)
            {
                f << std::setfill(' ') << std::setw(12) << std::setprecision(6) << hit.x
                  << std::setfill(' ') << std::setw(12) << std::setprecision(6) << hit.y
                  << std::setfill(' ') << std::setw(12) << std::setprecision(6) << hit.z;
            }
            f << std::endl;
        }
        f.close();
    }
};
