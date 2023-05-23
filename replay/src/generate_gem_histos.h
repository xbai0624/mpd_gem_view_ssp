#ifndef GENERATE_GEM_HISTOS_H
#define GENERATE_GEM_HISTOS_H

#include "histos.hpp"
#include "GEMSystem.h"
#include "GEMDetector.h"
#include "GEMPlane.h"
#include "TrackingDataHandler.h"
#include "AbstractDetector.h"
#include "tracking_struct.h"
#include "Tracking.h"
#include <vector>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>

namespace quality_check_histos
{
    // global variables
    static histos::HistoManager<> histM;
    static GEMSystem *gem_sys;
    static tracking_dev::TrackingDataHandler *tracking_data_handler;
    static tracking_dev::Tracking *tracking;
    static std::vector<tracking_dev::AbstractDetector*> fDet;

    static int NDetector_Implemented = 0;

    // 
    void pass_handles(GEMSystem *sys, tracking_dev::TrackingDataHandler *handle);
    void fill_gem_histos();
    void raw_histos();
    void tracking_histos();
    void save_histos();

    // helpers
    float get_strip_mean_time(const StripHit &h);
    float get_seed_strip_mean_time(const StripCluster &c);
};

#endif



#ifdef GENERATE_GEM_HISTOS_CXX


////////////////////////////////////////////////////////////////////////////////
//                      data quality check histos                             //
////////////////////////////////////////////////////////////////////////////////


namespace quality_check_histos
{
    ///////////////////////////////////////////////////////////////////////////
    // setup
    ///////////////////////////////////////////////////////////////////////////

    void pass_handles(GEMSystem *sys, tracking_dev::TrackingDataHandler *handle)
    {
        gem_sys = sys;
        tracking_data_handler = handle;
        tracking = tracking_data_handler -> GetTrackingHandle();
        NDetector_Implemented = tracking_data_handler -> GetNumberofDetectors();
        fDet.resize(NDetector_Implemented);
        for(int i=0; i<NDetector_Implemented; i++)
        {
            fDet[i] =  tracking_data_handler->GetDetector(i);
        }

        histM.init();
    }


    ///////////////////////////////////////////////////////////////////////////
    // entrance point
    ///////////////////////////////////////////////////////////////////////////

    void fill_gem_histos()
    {
        raw_histos();
        tracking_histos();
    }


    ///////////////////////////////////////////////////////////////////////////
    // gem raw data histograms
    ///////////////////////////////////////////////////////////////////////////

    // data quality check ROOT histograms before tracking
    void raw_histos()
    {
        // get all detectors
        std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();

        // loop through all detectors
        for(auto &det: detectors)
        {
            int layer = det -> GetLayerID();
            if(layer <0 || layer > 4) std::cout<<"Invalid layer = "<<layer<<std::endl;
            GEMPlane *pln_x = det -> GetPlane(GEMPlane::Plane_X);
            GEMPlane *pln_y = det -> GetPlane(GEMPlane::Plane_Y);

            const std::vector<StripHit> &x_strip_hits = pln_x -> GetStripHits();
            const std::vector<StripHit> &y_strip_hits = pln_y -> GetStripHits();
            int xs = (int)x_strip_hits.size(), ys = (int)y_strip_hits.size();

            // raw occupancy
            histM.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 0)) -> Fill(layer, xs);
            histM.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 1)) -> Fill(layer, ys);

            histM.histo_2d<float>(Form("h_raw_occupancy_plane%d", 0)) -> Fill(layer, xs/256.);
            histM.histo_2d<float>(Form("h_raw_occupancy_plane%d", 1)) -> Fill(layer, ys/256.);

            // strip info x axis
            for(auto &i: x_strip_hits) {
                histM.histo_1d<float>(Form("h_raw_xstrip_maxtimebin_layer%d", layer)) -> Fill(i.max_timebin);
                histM.histo_1d<float>(Form("h_raw_xstrip_adc_layer%d", layer)) -> Fill(i.charge);
                histM.histo_1d<float>(Form("h_raw_strip_mean_time_x_layer%d", layer)) -> Fill(get_strip_mean_time(i));
                histM.histo_2d<float>(Form("h_raw_x_strip_adc_index_layer%d", layer)) -> Fill(i.strip, i.charge);
            }

            // strip info y axis
            for(auto &i: y_strip_hits) {
                histM.histo_1d<float>(Form("h_raw_ystrip_maxtimebin_layer%d", layer)) -> Fill(i.max_timebin);
                histM.histo_1d<float>(Form("h_raw_ystrip_adc_layer%d", layer)) -> Fill(i.charge);
                histM.histo_1d<float>(Form("h_raw_strip_mean_time_y_layer%d", layer)) -> Fill(get_strip_mean_time(i));
                histM.histo_2d<float>(Form("h_raw_y_strip_adc_index_layer%d", layer)) -> Fill(i.strip, i.charge);
            }

            // clusters
            std::vector<StripCluster> & x_strip_cluster_ = pln_x -> GetStripClusters();
            std::vector<StripCluster> & y_strip_cluster_ = pln_y -> GetStripClusters();
            // make a copy
            std::vector<StripCluster> x_clusters = x_strip_cluster_;
            std::vector<StripCluster> y_clusters = y_strip_cluster_;

            // match x-y clusters according to their ADC values (tracking has its own histograms for this, which is more correct)
            std::sort(x_clusters.begin(), x_clusters.end(), [&](const StripCluster &c1, const StripCluster &c2) {
                    //return c1.total_charge > c2.total_charge;
                    return c1.peak_charge > c2.peak_charge;
                    });
            size_t c_s = x_clusters.size() < y_clusters.size() ? x_clusters.size() : y_clusters.size();
            for(size_t i=0; i<c_s; i++) {
                histM.histo_2d<float>(Form("h_raw_charge_correlation_layer%d", layer)) -> Fill(x_clusters[i].peak_charge, y_clusters[i].peak_charge);
                histM.histo_2d<float>(Form("h_raw_size_correlation_layer%d", layer)) -> Fill(x_clusters[i].hits.size(), y_clusters[i].hits.size());
                histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", layer)) -> Fill(x_clusters[i].position, y_clusters[i].position);
                histM.histo_2d<float>(Form("h_raw_seed_strip_mean_time_corr_layer%d", layer)) -> Fill(get_seed_strip_mean_time(x_clusters[i]), get_seed_strip_mean_time(y_clusters[i]));
            }
            // x side clusters
            for(auto &i: x_clusters) {
                histM.histo_1d<float>(Form("h_raw_cluster_adc_x_layer%d", layer)) -> Fill(i.peak_charge);
                histM.histo_1d<float>(Form("h_raw_cluster_size_x_layer%d", layer)) -> Fill(i.hits.size());
                histM.histo_1d<float>(Form("h_raw_cluster_pos_x_layer%d", layer)) -> Fill(i.position);
            }
            // y side clusters
            for(auto &i: y_clusters) {
                histM.histo_1d<float>(Form("h_raw_cluster_adc_y_layer%d", layer)) -> Fill(i.peak_charge);
                histM.histo_1d<float>(Form("h_raw_cluster_size_y_layer%d", layer)) -> Fill(i.hits.size());
                histM.histo_1d<float>(Form("h_raw_cluster_pos_y_layer%d", layer)) -> Fill(i.position);
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    // tracking based histograms
    ///////////////////////////////////////////////////////////////////////////

    // data quality check ROOT histograms with tracking
    void tracking_histos()
    {
        double xt, yt, xp, yp, chi2ndf;
        bool found_track = tracking -> GetBestTrack(xt, yt, xp, yp, chi2ndf);

        if(!found_track) {
            histM.histo_1d<float>("h_ntracks_found") -> Fill(0);
            histM.histo_1d<float>("h_nhits_on_best_track") -> Fill(0);
            return;
        }

        histM.histo_1d<float>("h_ntracks_found") -> Fill(tracking -> GetNGoodTrackCandidates());
        histM.histo_1d<float>("h_nhits_on_best_track") -> Fill(tracking -> GetNHitsonBestTrack());

        // fill best track to histograms
        histM.histo_1d<float>("h_xtrack") -> Fill(xt);
        histM.histo_1d<float>("h_ytrack") -> Fill(yt);
        histM.histo_1d<float>("h_xptrack") -> Fill(xp);
        histM.histo_1d<float>("h_yptrack") -> Fill(yp);
        histM.histo_1d<float>("h_chi2ndf") -> Fill(chi2ndf);

        tracking_dev::point_t pt(xt, yt, 0);
        tracking_dev::point_t dir(xp, yp, 1.);

        // get offset
        const std::vector<int> & layer_index = tracking -> GetBestTrackLayerIndex();
        const std::vector<int> & hit_index = tracking -> GetBestTrackHitIndex();
        for(unsigned int i=0; i<layer_index.size(); i++)
        {
            int layer = layer_index[i];
            int hit_id = hit_index[i];

            tracking_dev::point_t p = fDet[layer] -> Get2DHit(hit_id);
            histM.histo_1d<float>(Form("h_max_timebin_x_plane_gem%d", layer)) -> Fill(p.x_max_timebin);
            histM.histo_1d<float>(Form("h_max_timebin_y_plane_gem%d", layer)) -> Fill(p.y_max_timebin);
            histM.histo_1d<float>(Form("h_cluster_size_x_plane_gem%d", layer)) -> Fill(p.x_size);
            histM.histo_1d<float>(Form("h_cluster_size_y_plane_gem%d", layer)) -> Fill(p.y_size);
            histM.histo_1d<float>(Form("h_cluster_adc_x_plane_gem%d", layer)) -> Fill(p.x_peak);
            histM.histo_1d<float>(Form("h_cluster_adc_y_plane_gem%d", layer)) -> Fill(p.y_peak);

            fDet[layer] -> AddRealHits(p);
        }

        for(int i=0; i<NDetector_Implemented; i++)
        {
            tracking_dev::point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, fDet[i]->GetZPosition());
            fDet[i] -> AddFittedHits(p);
        }

        int n_good_track_candidates = tracking -> GetNGoodTrackCandidates();

        if(n_good_track_candidates > 0)
            histM.histo_1d<float>("h_ntrack_candidates") -> Fill(n_good_track_candidates);

        for(int i=0; i<NDetector_Implemented; i++)
        {
            const std::vector<tracking_dev::point_t> & real_hits = fDet[i] -> GetRealHits();
            const std::vector<tracking_dev::point_t> & fitted_hits = fDet[i] -> GetFittedHits();

            for(unsigned int hitid=0; hitid<real_hits.size(); hitid++)
            {
                histM.histo_1d<float>(Form("h_x_offset_gem%d", i)) -> Fill(real_hits[hitid].x - fitted_hits[hitid].x);
                histM.histo_1d<float>(Form("h_y_offset_gem%d", i)) -> Fill(real_hits[hitid].y - fitted_hits[hitid].y);

                histM.histo_2d<float>(Form("h_didhit_xy_gem%d", i)) -> Fill(real_hits[hitid].x, real_hits[hitid].y);
            }

            for(auto &h: fitted_hits) {
                histM.histo_2d<float>(Form("h_shouldhit_xy_gem%d", i)) -> Fill(h.x, h.y);
            }
        }

        // generate gem tracking based efficiency plots
        for(int i=0; i<NDetector_Implemented; i++)
        {
            for(int xbins = 1; xbins < 120; xbins++)
            {
                for(int ybins = 1; ybins < 120; ybins++) {
                    float did = histM.histo_2d<float>((Form("h_didhit_xy_gem%d", i))) -> GetBinContent(xbins, ybins);
                    float should = histM.histo_2d<float>((Form("h_shouldhit_xy_gem%d", i))) -> GetBinContent(xbins, ybins);

                    float eff = 0;
                    if(should >0) eff = (did / should < 1) ? did / should : 1.;
                    histM.histo_2d<float>(Form("h_2defficiency_xy_gem%d", i)) -> SetBinContent(xbins, ybins, eff);
                }
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    // write histograms to disks
    ///////////////////////////////////////////////////////////////////////////

    void save_histos()
    {
        std::string path = "Rootfiles/data_quality_check.root";
        std::cout<<"Writing data quality check histograms to : "<<path<<std::endl;
        histM.save(path.c_str());
    }

    ///////////////////////////////////////////////////////////////////////////
    // helpers
    ///////////////////////////////////////////////////////////////////////////

    float get_strip_mean_time(const StripHit& s){
        int ts = (int)s.ts_adc.size();
        float adc = 0;
        float time_adc = 0;
        for(int i=0; i<ts; i++) {
            adc += s.ts_adc[i];
            time_adc += s.ts_adc[i] * 25. * (i+1);
        }
        if(adc > 0)
            return time_adc / adc;
        return 0;
    };

    float get_seed_strip_mean_time(const StripCluster &c){
        float res = -1;
        float adc = -9999999;
        for(auto &i: c.hits) {
            float tmp = get_strip_mean_time(i);
            if(i.charge > adc) {
                adc = i.charge;
                res = tmp;
            }
        }
        return res;
    };

}

#endif
