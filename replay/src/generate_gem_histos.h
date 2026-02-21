#ifndef GENERATE_GEM_HISTOS_H
#define GENERATE_GEM_HISTOS_H

#include "histos.hpp"
#include "GEMSystem.h"
#include "GEMDetector.h"
#include "GEMPlane.h"
#include "TrackingDataHandler.h"
#include "VirtualDetector.h"
#include "tracking_struct.h"
#include "Tracking.h"
#include "MollerROStripDesign.hpp"
#include "log_tracks.hpp"
#include "Cuts.h"
#include <vector>
#include <utility>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TMath.h>

#define WRITE_TRACKS_TO_DISK

namespace quality_check_histos
{
    // global variables
    static histos::HistoManager<> histM;
    static GEMSystem *gem_sys;
    static tracking_dev::TrackingDataHandler *tracking_data_handler;
    static tracking_dev::Tracking *tracking;
    static std::unordered_map<int, tracking_dev::VirtualDetector*> fDet;
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
    std::pair<double, double> convert_uv_to_xy_fit_cylindrical(const double &x, const double &w);
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
        fDet = tracking_data_handler -> GetDetectorList();

        histM.init();
#ifdef WRITE_TRACKS_TO_DISK
        log_tracks::open_tracks_text_file();
#endif
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
        load_moller_strip_design_files();

        // event number
        histM.histo_1d<float>("h_event_number") -> Fill((float)event_number);

        // loop through all detectors
        for(auto &det: detectors)
        {
            int det_module_id = det -> GetDetID();
            GEMPlane *pln_x = det -> GetPlane(GEMPlane::Plane_X);
            GEMPlane *pln_y = det -> GetPlane(GEMPlane::Plane_Y);

            // one APV has 128 channels
            double x_total_strips = pln_x -> GetCapacity() * 128;
            double y_total_strips = pln_y -> GetCapacity() * 128;

            const std::vector<StripHit> &x_strip_hits = pln_x -> GetStripHits();
            const std::vector<StripHit> &y_strip_hits = pln_y -> GetStripHits();
            int xs = (int)x_strip_hits.size(), ys = (int)y_strip_hits.size();

            // raw occupancy
            histM.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 0)) -> Fill(det_module_id, xs);
            histM.histo_2d<float>(Form("h_raw_fired_strip_plane%d", 1)) -> Fill(det_module_id, ys);

            histM.histo_2d<float>(Form("h_raw_occupancy_plane%d", 0)) -> Fill(det_module_id, xs/x_total_strips);
            histM.histo_2d<float>(Form("h_raw_occupancy_plane%d", 1)) -> Fill(det_module_id, ys/y_total_strips);

            // strip info x axis
            for(auto &i: x_strip_hits) {
                histM.histo_1d<float>(Form("h_raw_xstrip_maxtimebin_layer%d", det_module_id)) -> Fill(i.max_timebin);
                histM.histo_1d<float>(Form("h_raw_xstrip_adc_layer%d", det_module_id)) -> Fill(i.charge);
                histM.histo_1d<float>(Form("h_raw_strip_mean_time_x_layer%d", det_module_id)) -> Fill(get_strip_mean_time(i));
                histM.histo_2d<float>(Form("h_raw_x_strip_adc_index_layer%d", det_module_id)) -> Fill(i.strip, i.charge);
            }

            // strip info y axis
            for(auto &i: y_strip_hits) {
                histM.histo_1d<float>(Form("h_raw_ystrip_maxtimebin_layer%d", det_module_id)) -> Fill(i.max_timebin);
                histM.histo_1d<float>(Form("h_raw_ystrip_adc_layer%d", det_module_id)) -> Fill(i.charge);
                histM.histo_1d<float>(Form("h_raw_strip_mean_time_y_layer%d", det_module_id)) -> Fill(get_strip_mean_time(i));
                histM.histo_2d<float>(Form("h_raw_y_strip_adc_index_layer%d", det_module_id)) -> Fill(i.strip, i.charge);
            }

            // clusters
            std::vector<StripCluster> & x_strip_cluster_ = pln_x -> GetStripClusters();
            std::vector<StripCluster> & y_strip_cluster_ = pln_y -> GetStripClusters();
            // make a copy
            std::vector<StripCluster> x_clusters = x_strip_cluster_;
            std::vector<StripCluster> y_clusters = y_strip_cluster_;

            // cluster multiplicity
            if(x_strip_cluster_.size() > 0)
                histM.histo_1d<float>(Form("h_raw_x_cluster_multiplicity_layer%d", det_module_id)) -> Fill(x_strip_cluster_.size());
            if(y_strip_cluster_.size() > 0)
                histM.histo_1d<float>(Form("h_raw_y_cluster_multiplicity_layer%d", det_module_id)) -> Fill(y_strip_cluster_.size());

            // match x-y clusters according to their ADC values (tracking has its own histograms for this, which is more correct)
            std::sort(x_clusters.begin(), x_clusters.end(), [&](const StripCluster &c1, const StripCluster &c2) {
                    //return c1.total_charge > c2.total_charge;
                    return c1.peak_charge > c2.peak_charge;
                    });
            std::sort(y_clusters.begin(), y_clusters.end(), [&](const StripCluster &c1, const StripCluster &c2) {
                    //return c1.total_charge > c2.total_charge;
                    return c1.peak_charge > c2.peak_charge;
                    });
            size_t c_s = x_clusters.size() < y_clusters.size() ? x_clusters.size() : y_clusters.size();

            for(size_t i=0; i<c_s; i++) {
                histM.histo_2d<float>(Form("h_raw_charge_correlation_layer%d", det_module_id)) -> Fill(x_clusters[i].peak_charge, y_clusters[i].peak_charge);
                histM.histo_2d<float>(Form("h_raw_size_correlation_layer%d", det_module_id)) -> Fill(x_clusters[i].hits.size(), y_clusters[i].hits.size());
                histM.histo_2d<float>(Form("h_raw_seed_strip_mean_time_corr_layer%d", det_module_id)) -> Fill(get_seed_strip_mean_time(x_clusters[i]), get_seed_strip_mean_time(y_clusters[i]));

				if(det->GetType() == "MOLLERGEM")
                {
					auto &x_strips = x_clusters[i].hits;
					auto &y_strips = y_clusters[i].hits;
					bool has_intersect = false;
					for(auto &x_s_i: x_strips) {
						int x_s_nb = x_s_i.strip;
						for(auto &y_s_i: y_strips) {
							int y_s_nb = y_s_i.strip;
							bool _tmp_intersect = has_intersect_bot_top(x_s_nb, y_s_nb);
							if(_tmp_intersect)
								has_intersect = _tmp_intersect;
						}
					}
					if(has_intersect) {
						auto p2d = convert_uv_to_xy_moller(x_clusters[i].position, y_clusters[i].position);
						histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", det_module_id)) -> Fill(p2d.first, p2d.second);
					}
				}
                else if( det -> GetType() == "FITCYLINDRICAL")
                {
                    auto p2d = convert_uv_to_xy_fit_cylindrical(x_clusters[i].position, y_clusters[i].position);

                    double half_x_length = 365.6/2.; // mm
                    double half_y_length = 365.6/2.; // mm

                    //if( p2d.first < half_x_length && p2d.first > -half_x_length &&
                    //        p2d.second < half_y_length && p2d.second > -half_y_length)
                    {
                        histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", det_module_id)) -> Fill(p2d.first, p2d.second);
                    }
                }
				else if( (det -> GetType() == "INFNXWGEM") || (det -> GetType() == "UVAXWGEM") )
				{
					auto p2d = convert_xw_to_xy_sbs(x_clusters[i].position, y_clusters[i].position);
					histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", det_module_id)) -> Fill(p2d.first, p2d.second);
				}
				else {
					histM.histo_2d<float>(Form("h_raw_pos_correlation_layer%d", det_module_id)) -> Fill(x_clusters[i].position, y_clusters[i].position);
				}
			}
			// x side clusters
			for(auto &i: x_clusters) {
				histM.histo_1d<float>(Form("h_raw_cluster_adc_x_layer%d", det_module_id)) -> Fill(i.peak_charge);
				histM.histo_1d<float>(Form("h_raw_cluster_size_x_layer%d", det_module_id)) -> Fill(i.hits.size());
				histM.histo_1d<float>(Form("h_raw_cluster_pos_x_layer%d", det_module_id)) -> Fill(i.position);
			}
			// y side clusters
			for(auto &i: y_clusters) {
				histM.histo_1d<float>(Form("h_raw_cluster_adc_y_layer%d", det_module_id)) -> Fill(i.peak_charge);
				histM.histo_1d<float>(Form("h_raw_cluster_size_y_layer%d", det_module_id)) -> Fill(i.hits.size());
				histM.histo_1d<float>(Form("h_raw_cluster_pos_y_layer%d", det_module_id)) -> Fill(i.position);
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

		tracking_dev::point_t pt(xt, yt, 0);
		tracking_dev::point_t dir(xp, yp, 1.);

		if(!found_track) {
			histM.histo_1d<float>("h_ntracks_found") -> Fill(0);
			//histM.histo_1d<float>("h_nhits_on_best_track") -> Fill(0);
			return;
		}
        for(auto &it: fDet) {
            const tracking_dev::point_t & origin = it.second -> GetOrigin();
            const tracking_dev::point_t & dimension = it.second -> GetDimension();
			tracking_dev::point_t projected_hit = tracking->GetTrackingUtility() -> projected_point(pt, dir, it.second->GetZPosition());

            // fitted hit must fall within the detector acceptance
            if(projected_hit.x < (origin.x - dimension.x/2) || projected_hit.x > (origin.x + dimension.x/2))
                return;

            if(projected_hit.y < (origin.y - dimension.y/2) || projected_hit.y > (origin.y + dimension.y/2))
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

		// part 1)
		// get inclusive residue
        const std::vector<tracking_dev::point_t>& hits_on_best_track = tracking -> GetVHitsOnBestTrack();

#ifdef WRITE_TRACKS_TO_DISK
        log_tracks::append_track(hits_on_best_track);
#endif

		for(auto &it: hits_on_best_track)
		{
			int module_id = it.module_id;

			histM.histo_1d<float>(Form("h_max_timebin_x_plane_gem%d", module_id)) -> Fill(it.x_max_timebin);
			histM.histo_1d<float>(Form("h_max_timebin_y_plane_gem%d", module_id)) -> Fill(it.y_max_timebin);
			histM.histo_1d<float>(Form("h_cluster_size_x_plane_gem%d", module_id)) -> Fill(it.x_size);
			histM.histo_1d<float>(Form("h_cluster_size_y_plane_gem%d", module_id)) -> Fill(it.y_size);
			histM.histo_1d<float>(Form("h_cluster_adc_x_plane_gem%d", module_id)) -> Fill(it.x_peak);
			histM.histo_1d<float>(Form("h_cluster_adc_y_plane_gem%d", module_id)) -> Fill(it.y_peak);
		}

		int n_good_track_candidates = tracking -> GetNGoodTrackCandidates();

		if(n_good_track_candidates > 0)
			histM.histo_1d<float>("h_ntrack_candidates") -> Fill(n_good_track_candidates);

		for(auto &it: fDet)
		{
            int det_module_id = it.second -> GetDetModuleID();

            tracking_dev::point_t hit_did_hit;
            bool fired = false;
            for(auto &i: hits_on_best_track) {
                if(i.module_id == det_module_id) {
                    fired = true;
                    hit_did_hit = i;
                }
            }

			tracking_dev::point_t hit_should_hit = tracking->GetTrackingUtility() -> projected_point(pt, dir, it.second->GetZPosition());
 
			if(fired)
			{
				histM.histo_1d<float>(Form("h_xresid_gem%d_inclusive", det_module_id)) -> Fill(hit_did_hit.x - hit_should_hit.x);
				histM.histo_1d<float>(Form("h_yresid_gem%d_inclusive", det_module_id)) -> Fill(hit_did_hit.y - hit_should_hit.y);

				histM.histo_2d<float>(Form("h_didhit_xy_gem%d", det_module_id)) -> Fill(hit_did_hit.x, hit_did_hit.y);
				histM.histo_1d<float>(Form("h_didhit_x_gem%d", det_module_id)) -> Fill(hit_did_hit.x);
				histM.histo_1d<float>(Form("h_didhit_y_gem%d", det_module_id)) -> Fill(hit_did_hit.y);
			}

            histM.histo_2d<float>(Form("h_shouldhit_xy_gem%d", det_module_id)) -> Fill(hit_should_hit.x, hit_should_hit.y);
		}

        // fill did_hit and should_hit 2D histograms -- for tracking based efficiency study
		for(auto &it: fDet)
		{
            int det_module_id = it.second -> GetDetModuleID();

            // a valid track should have at least 3 hits
            if(hits_on_best_track.size() < 3)
                continue;
 
            bool det_fired = false;
            for(auto &i: hits_on_best_track) {
                if( i.module_id == det_module_id)
                    det_fired = true;
            }
            // if the track has only 3 hits, and this detector is one of them, then we cannot use this track for efficiency calculation
            if(det_fired && hits_on_best_track.size() == 3)
                continue;
 
            tracking_dev::point_t hit_did_hit;
            bool fired = false;
            for(auto &i: hits_on_best_track) {
                if(i.module_id == det_module_id) {
                    fired = true;
                    hit_did_hit = i;
                }
            }

			tracking_dev::point_t hit_should_hit = tracking->GetTrackingUtility() -> projected_point(pt, dir, it.second->GetZPosition());
 
			if(fired)
			{
				histM.histo_2d<float>(Form("h_for_efficiency_didhit_xy_gem%d", det_module_id)) -> Fill(hit_did_hit.x, hit_did_hit.y);
			}

            histM.histo_2d<float>(Form("h_for_efficiency_shouldhit_xy_gem%d", det_module_id)) -> Fill(hit_should_hit.x, hit_should_hit.y);
		}

		// part 2)
		// get exclusive residue plots
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
				int _module_id = hits_on_best_track[_ihit].module_id;
				tracking_dev::point_t _pt(xtrack, ytrack, 0);
				tracking_dev::point_t _dir(xptrack, yptrack, 1.);
				tracking_dev::point_t _p = tracking->GetTrackingUtility() -> projected_point(_pt, _dir, fDet[_module_id]->GetZPosition());

				// get exclusive residue
				double x_exclusive_d = _p.x - hits_on_best_track[_ihit].x;
				double y_exclusive_d = _p.y - hits_on_best_track[_ihit].y;
				histM.histo_1d<float>(Form("h_xresid_gem%d_exclusive", _module_id)) -> Fill(x_exclusive_d);
				histM.histo_1d<float>(Form("h_yresid_gem%d_exclusive", _module_id)) -> Fill(y_exclusive_d);

				// get residue vs x/y position 2d plots
				histM.histo_2d<float>(Form("h_xresid_x_did_hit_gem%d_exclusive", _module_id)) -> Fill(hits_on_best_track[_ihit].x, x_exclusive_d);
				histM.histo_2d<float>(Form("h_xresid_y_did_hit_gem%d_exclusive", _module_id)) -> Fill(hits_on_best_track[_ihit].y, x_exclusive_d);
				histM.histo_2d<float>(Form("h_xresid_x_should_hit_gem%d_exclusive", _module_id)) -> Fill(_p.x, x_exclusive_d);
				histM.histo_2d<float>(Form("h_xresid_y_should_hit_gem%d_exclusive", _module_id)) -> Fill(_p.y, x_exclusive_d);
				histM.histo_2d<float>(Form("h_yresid_x_did_hit_gem%d_exclusive", _module_id)) -> Fill(hits_on_best_track[_ihit].x, y_exclusive_d);
				histM.histo_2d<float>(Form("h_yresid_y_did_hit_gem%d_exclusive", _module_id)) -> Fill(hits_on_best_track[_ihit].y, y_exclusive_d);
				histM.histo_2d<float>(Form("h_yresid_x_should_hit_gem%d_exclusive", _module_id)) -> Fill(_p.x, y_exclusive_d);
				histM.histo_2d<float>(Form("h_yresid_y_should_hit_gem%d_exclusive", _module_id)) -> Fill(_p.y, y_exclusive_d);
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
                float search_radius = Cuts::Instance().__get("effective search radius").val<float>();
                size_t total_2d_hits = det.second -> Get2DHitCounts();

                // since each 2D hit might have different z position due to rotation
                // so we have to do the projection point by point
                double r = 99999999;
                double xresidue = r, yresidue = r, x_did_hit = r, y_did_hit = r;

                for(size_t i=0; i<total_2d_hits; i++) {
                    tracking_dev::point_t p_i  = det.second -> Get2DHit(i);
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

                int _module_id = det.second -> GetDetModuleID();

                // 1d should hit -tracker based histograms
                double z_gem = det.second -> GetOrigin().z;
                tracking_dev::point_t p = tracking->GetTrackingUtility() -> projected_point(pt, dir, z_gem);
                histM.histo_1d<float>(Form("h_xshould_hit_tracker_based_gem%d", _module_id)) -> Fill(p.x);
                histM.histo_1d<float>(Form("h_yshould_hit_tracker_based_gem%d", _module_id)) -> Fill(p.y);

                if( r < 99999999 )
                {
                    histM.histo_1d<float>(Form("h_xresidue_gem%d_tracker_exclusive", _module_id)) -> Fill(xresidue);
                    histM.histo_1d<float>(Form("h_yresidue_gem%d_tracker_exclusive", _module_id)) -> Fill(yresidue);

                    histM.histo_2d<float>(Form("h_xresidue_x_did_hit_gem%d_tracker_exclusive", _module_id)) -> Fill(x_did_hit, xresidue);
                    histM.histo_2d<float>(Form("h_xresidue_y_did_hit_gem%d_tracker_exclusive", _module_id)) -> Fill(y_did_hit, xresidue);
                    histM.histo_2d<float>(Form("h_yresidue_x_did_hit_gem%d_tracker_exclusive", _module_id)) -> Fill(x_did_hit, yresidue);
                    histM.histo_2d<float>(Form("h_yresidue_y_did_hit_gem%d_tracker_exclusive", _module_id)) -> Fill(y_did_hit, yresidue);

                    // 1d did hit -tracker based histograms
                    histM.histo_1d<float>(Form("h_xdid_hit_tracker_based_gem%d", _module_id)) -> Fill(x_did_hit);
                    histM.histo_1d<float>(Form("h_ydid_hit_tracker_based_gem%d", _module_id)) -> Fill(y_did_hit);
                }
            }
        }
    }

    void generate_tracking_based_2d_efficiency_plots()
    {
        // generate gem tracking based efficiency plots

        const std::vector<int> &det_module_ids = tracking_data_handler -> GetDetectorModuleIDs();
        for(auto &i: det_module_ids) {
            int NXBINS_did_hit = histM.histo_2d<float>((Form("h_didhit_xy_gem%d", i)))->GetNbinsX();
            int NXBINS_should_hit = histM.histo_2d<float>((Form("h_shouldhit_xy_gem%d", i)))->GetNbinsX();
            if(NXBINS_did_hit != NXBINS_should_hit) {
                std::cout<<"The X Bins for did_hit histos should be the same with should_hit histos, checking config file histo.conf"<<std::endl;
                exit(0);
            }
            int NYBINS_did_hit = histM.histo_2d<float>((Form("h_didhit_xy_gem%d", i)))->GetNbinsY();
            int NYBINS_should_hit = histM.histo_2d<float>((Form("h_shouldhit_xy_gem%d", i)))->GetNbinsY();
            if(NYBINS_did_hit != NYBINS_should_hit) {
                std::cout<<"The Y Bins for did_hit histos should be the same with should_hit histos, checking config file histo.conf"<<std::endl;
                exit(0);
            }

            for(int xbins = 1; xbins < NXBINS_did_hit; xbins++)
            {
                for(int ybins = 1; ybins < NYBINS_did_hit; ybins++) {
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

        //u += 20;
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

    std::pair<double, double> convert_uv_to_xy_fit_cylindrical(const double &u, const double &v)
    {
        //return std::make_pair(u, v);

        // FIT Cylindircal strip angle is 45 degree, for both U and V strips
        double angle = 45 * 3.1415926 / 180.;
        double x = 0, y = 0; 

        x = TMath::Cos(angle) * u - TMath::Sin(angle) * v;
        y = TMath::Sin(angle) * u + TMath::Cos(angle) * v;

        return std::make_pair(x, y);
    }

}

#endif
