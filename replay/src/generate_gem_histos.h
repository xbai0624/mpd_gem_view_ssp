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
#include <utility>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TMath.h>

namespace quality_check_histos
{
    // global variables
    static histos::HistoManager<> histM;
    static GEMSystem *gem_sys;
    static Cuts *tracking_cuts;
    static tracking_dev::TrackingDataHandler *tracking_data_handler;
    static tracking_dev::Tracking *tracking;
    static std::vector<tracking_dev::AbstractDetector*> fDet;
    static std::string output_file_name;

    static int NDetector_Implemented = 0;

    // 
    void pass_handles(GEMSystem *sys, tracking_dev::TrackingDataHandler *handle);
    void set_output_name(std::string name);
    void fill_gem_histos(int event_number);
    void raw_histos(int event_number);
    void tracking_histos();
    void generate_tracking_based_2d_efficiency_plots();
    void save_histos();

    // helpers
    float get_strip_mean_time(const StripHit &h);
    float get_seed_strip_mean_time(const StripCluster &c);
    std::pair<double, double> convert_uv_to_xy_moller(const double &u, const double &v);
    std::pair<double, double> convert_xw_to_xy_sbs(const double &x, const double &w);
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
        tracking_cuts = tracking -> GetTrackingCuts();
        NDetector_Implemented = tracking_data_handler -> GetNumberofDetectors();
        fDet.resize(NDetector_Implemented);
        for(int i=0; i<NDetector_Implemented; i++)
        {
            fDet[i] =  tracking_data_handler->GetDetector(i);
        }

        histM.init();
    }

    void set_output_name(std::string name)
    {
        output_file_name = name;
    }


    ///////////////////////////////////////////////////////////////////////////
    // entrance point
    ///////////////////////////////////////////////////////////////////////////

    void fill_gem_histos(int event_number)
    {
        raw_histos(event_number);
        tracking_histos();
    }


    ///////////////////////////////////////////////////////////////////////////
    // gem raw data histograms
    ///////////////////////////////////////////////////////////////////////////

    // data quality check ROOT histograms before tracking
    void raw_histos(int event_number)
    {
        // get all detectors
        if(!gem_sys) {
            std::cout<<"Null gem_sys"<<std::endl;
        }
        std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();

        // event number
        histM.histo_1d<float>("h_event_number") -> Fill((float)event_number);

        // loop through all detectors
        for(auto &det: detectors)
        {
            int layer = det -> GetLayerID();
            if(layer <0 || layer > 7) std::cout<<"Invalid layer = "<<layer<<std::endl;
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

            // cluster multiplicity
            if(x_strip_cluster_.size() > 0)
                histM.histo_1d<float>(Form("h_raw_x_cluster_multiplicity_layer%d", layer)) -> Fill(x_strip_cluster_.size());
            if(y_strip_cluster_.size() > 0)
                histM.histo_1d<float>(Form("h_raw_y_cluster_multiplicity_layer%d", layer)) -> Fill(y_strip_cluster_.size());

            // match x-y clusters according to their ADC values (tracking has its own histograms for this, which is more correct)
            std::sort(x_clusters.begin(), x_clusters.end(), [&](const StripCluster &c1, const StripCluster &c2) {
                    //return c1.total_charge > c2.total_charge;
                    return c1.peak_charge > c2.peak_charge;
                    });
            size_t c_s = x_clusters.size() < y_clusters.size() ? x_clusters.size() : y_clusters.size();

            for(size_t i=0; i<c_s; i++) {
                histM.histo_2d<float>(Form("h_raw_charge_correlation_layer%d", layer)) -> Fill(x_clusters[i].peak_charge, y_clusters[i].peak_charge);
                histM.histo_2d<float>(Form("h_raw_size_correlation_layer%d", layer)) -> Fill(x_clusters[i].hits.size(), y_clusters[i].hits.size());
                histM.histo_2d<float>(Form("h_raw_seed_strip_mean_time_corr_layer%d", layer)) -> Fill(get_seed_strip_mean_time(x_clusters[i]), get_seed_strip_mean_time(y_clusters[i]));

                if(det->GetType() == "MOLLERGEM") {
                    auto p2d = convert_uv_to_xy_moller(x_clusters[i].position, y_clusters[i].position);
                    histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", layer)) -> Fill(p2d.first, p2d.second);
                }
		else if( (det -> GetType() == "INFNXWGEM") || (det -> GetType() == "UVAXWGEM") )
		{
                    auto p2d = convert_xw_to_xy_sbs(x_clusters[i].position, y_clusters[i].position);
                    histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", layer)) -> Fill(p2d.first, p2d.second);
		}
                else {
                    histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", layer)) -> Fill(x_clusters[i].position, y_clusters[i].position);
		}
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
            //histM.histo_1d<float>("h_nhits_on_best_track") -> Fill(0);
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

        // part 1)
        // get inclusive residue
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
                histM.histo_1d<float>(Form("h_xresid_gem%d_inclusive", i)) -> Fill(real_hits[hitid].x - fitted_hits[hitid].x);
                histM.histo_1d<float>(Form("h_yresid_gem%d_inclusive", i)) -> Fill(real_hits[hitid].y - fitted_hits[hitid].y);

                histM.histo_2d<float>(Form("h_didhit_xy_gem%d", i)) -> Fill(real_hits[hitid].x, real_hits[hitid].y);
                histM.histo_1d<float>(Form("h_didhit_x_gem%d", i)) -> Fill(real_hits[hitid].x);
                histM.histo_1d<float>(Form("h_didhit_y_gem%d", i)) -> Fill(real_hits[hitid].y);
            }

            for(auto &h: fitted_hits) {
                histM.histo_2d<float>(Form("h_shouldhit_xy_gem%d", i)) -> Fill(h.x, h.y);
            }
        }

        // part 2)
        // get exclusive residue plots
        const std::vector<tracking_dev::point_t> &hits_on_best_track = tracking -> GetVHitsOnBestTrack();
        size_t n_total_hits_on_best_track = hits_on_best_track.size();
        // exclusive fitting must has minimum 4 (3 without current layer)
        if( ((int)n_total_hits_on_best_track - 1 ) >= 3) 
        {
            for(size_t _ihit = 0; _ihit<hits_on_best_track.size(); _ihit++)
            {
                std::vector<tracking_dev::point_t> temp;
                for(size_t ihit = 0; ihit < hits_on_best_track.size(); ihit++) {
                    if(ihit != _ihit)
                        temp.push_back(hits_on_best_track[ihit]);
                }

                // do a fit excluding the hit on the current layer
                double xtrack, ytrack, xptrack, yptrack, chi2ndf;
                std::vector<double> xresid, yresid;
                tracking -> GetTrackingUtility() -> FitLine(temp, xtrack, ytrack, xptrack, yptrack, chi2ndf, 
                        xresid, yresid);

                // project to the current detector
                int _layer = hits_on_best_track[_ihit].layer_id;
                tracking_dev::point_t _pt(xtrack, ytrack, 0);
                tracking_dev::point_t _dir(xptrack, yptrack, 1.);
                tracking_dev::point_t _p = tracking->GetTrackingUtility() -> projected_point(_pt, _dir, fDet[_layer]->GetZPosition());

                // get exclusive residue
                double x_exclusive_d = _p.x - hits_on_best_track[_ihit].x;
                double y_exclusive_d = _p.y - hits_on_best_track[_ihit].y;
                histM.histo_1d<float>(Form("h_xresid_gem%d_exclusive", _layer)) -> Fill(x_exclusive_d);
                histM.histo_1d<float>(Form("h_yresid_gem%d_exclusive", _layer)) -> Fill(y_exclusive_d);

                // get residue vs x/y position 2d plots
                histM.histo_2d<float>(Form("h_xresid_x_did_hit_gem%d_exclusive", _layer)) -> Fill(hits_on_best_track[_ihit].x, x_exclusive_d);
                histM.histo_2d<float>(Form("h_xresid_y_did_hit_gem%d_exclusive", _layer)) -> Fill(hits_on_best_track[_ihit].y, x_exclusive_d);
                histM.histo_2d<float>(Form("h_xresid_x_should_hit_gem%d_exclusive", _layer)) -> Fill(_p.x, x_exclusive_d);
                histM.histo_2d<float>(Form("h_xresid_y_should_hit_gem%d_exclusive", _layer)) -> Fill(_p.y, x_exclusive_d);
                histM.histo_2d<float>(Form("h_yresid_x_did_hit_gem%d_exclusive", _layer)) -> Fill(hits_on_best_track[_ihit].x, y_exclusive_d);
                histM.histo_2d<float>(Form("h_yresid_y_did_hit_gem%d_exclusive", _layer)) -> Fill(hits_on_best_track[_ihit].y, y_exclusive_d);
                histM.histo_2d<float>(Form("h_yresid_x_should_hit_gem%d_exclusive", _layer)) -> Fill(_p.x, y_exclusive_d);
                histM.histo_2d<float>(Form("h_yresid_y_should_hit_gem%d_exclusive", _layer)) -> Fill(_p.y, y_exclusive_d);
            }
        }

        // part 3)
        // get tracker-only based residue plots (in case you have non-tracker and tracker chambers)
        // ---- tracks are fitted using trackers only
        // ---- use the best track, project to non-tracker chambers, and find the residues,
        // ---- we will use the closest detected 2D hits on non-tracker chambers
        // ---- we will search thechamber for the closest hit
        // ---- search radius around the projected hit is configurable
        //
        // if you don't separate chambers into trackers and non-trackers,
        // then these plots will be equivalent to inclusive residue plots
        if(found_track) {
            for(auto &det: fDet) {
                // look for the closest 2d hit
                float search_radius = tracking_cuts->__get("effective search radius").val<float>();
                size_t total_2d_hits = det -> Get2DHitCounts();

                // since each 2D hit might have different z position due to rotation
                // so we have to do the projection point by point
                double r = 99999999;
                double xresidue = r, yresidue = r, x_did_hit = r, y_did_hit = r;

                for(size_t i=0; i<total_2d_hits; i++) {
                    tracking_dev::point_t p_i  = det -> Get2DHit(i);
                    tracking_dev::point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, p_i.z);
                    tracking_dev::point_t p_diff = p_i - p;

                    double distance = p_diff.mod();

                    // only search within the search_radius
                    if(distance > search_radius)
                        continue;

                    if(distance < r) {
                        r = distance;
                        xresidue = p_diff.x;
                        yresidue = p_diff.y;
                        x_did_hit = p_i.x;
                        y_did_hit = p_i.y;
                    }
                }

                int _layer = det -> GetLayerID();

                // 1d should hit -tracker based histograms
                double z_gem = det -> GetOrigin().z;
                tracking_dev::point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, z_gem);
                histM.histo_1d<float>(Form("h_xshould_hit_tracker_based_gem%d", _layer)) -> Fill(p.x);
                histM.histo_1d<float>(Form("h_yshould_hit_tracker_based_gem%d", _layer)) -> Fill(p.y);

                if( r < 99999999 )
                {
                    histM.histo_1d<float>(Form("h_xresidue_gem%d_tracker_exclusive", _layer)) -> Fill(xresidue);
                    histM.histo_1d<float>(Form("h_yresidue_gem%d_tracker_exclusive", _layer)) -> Fill(yresidue);

                    histM.histo_2d<float>(Form("h_xresidue_x_did_hit_gem%d_tracker_exclusive", _layer)) -> Fill(x_did_hit, xresidue);
                    histM.histo_2d<float>(Form("h_xresidue_y_did_hit_gem%d_tracker_exclusive", _layer)) -> Fill(y_did_hit, xresidue);
                    histM.histo_2d<float>(Form("h_yresidue_x_did_hit_gem%d_tracker_exclusive", _layer)) -> Fill(x_did_hit, yresidue);
                    histM.histo_2d<float>(Form("h_yresidue_y_did_hit_gem%d_tracker_exclusive", _layer)) -> Fill(y_did_hit, yresidue);

                    // 1d did hit -tracker based histograms
                    histM.histo_1d<float>(Form("h_xdid_hit_tracker_based_gem%d", _layer)) -> Fill(x_did_hit);
                    histM.histo_1d<float>(Form("h_ydid_hit_tracker_based_gem%d", _layer)) -> Fill(y_did_hit);
                }
            }
        }
    }

    void generate_tracking_based_2d_efficiency_plots()
    {
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
        std::string path = output_file_name + std::string("_data_quality_check.root");
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

    std::pair<double, double> convert_uv_to_xy_moller(const double &_u, const double &_v)
    {
        // moller strip angle is 26.5 degree
        double angle = 26.5 * 3.1415926 / 180.;
        double u = _u, v = _v;

        u += 20;
        v -= 406 * TMath::Sin(angle / 2.);
        double y = -0.5 * ( (u-v) / TMath::Tan(angle/2.) - 406);
        double x = 0.5 * ( u + v);

        return std::make_pair(x, y);
    }

    std::pair<double, double> convert_xw_to_xy_sbs(const double &_x, const double &_w)
    {
        // SBS W strip angle is 45 degree
        //double angle = 45 * 3.1415926 / 180.;
        double x = _x, w = _w;

	double y = w / TMath::Sqrt(2);

        return std::make_pair(x, y);
    }
}

#endif
