#include "GEMRootClusterTree.h"
#include "GEMStruct.h"
#include "GEMSystem.h"
#include "GEMCluster.h"
#include "APVStripMapping.h"

#include <iostream>

GEMRootClusterTree::GEMRootClusterTree(const char* path)
{
    fPath = path;
    pFile = new TFile(path, "RECREATE");
    pTree = new TTree("GEMCluster", "cluster list");

    pTree -> Branch("evtID", &evtID, "evtID/I"); 

    // GEM Tracking result
    pTree->Branch("fNtracks_found", &fNtracks_found, "fNtracks_found/I");
    pTree->Branch("fNAllGoodTrackCandidates", &fNAllGoodTrackCandidates, "fNAllGoodTrackCandidates/I");
    pTree->Branch("besttrack", &besttrack, "besttrack/I");
    pTree->Branch("fNhitsOnTrack", &fNhitsOnTrack);
    pTree->Branch("fXtrack", &fXtrack);
    pTree->Branch("fYtrack", &fYtrack);
    pTree->Branch("fXptrack", &fXptrack);
    pTree->Branch("fYptrack", &fYptrack);
    pTree->Branch("fChi2Track", &fChi2Track);
    pTree->Branch("fNgoodhits", &ngoodhits, "ngoodhits/I");
    pTree->Branch("fHitXlocal", &fHitXlocal);
    pTree->Branch("fHitYlocal", &fHitYlocal);
    pTree->Branch("fHitZlocal", &fHitZlocal);
    pTree->Branch("fHitTrackIndex", &hit_track_index);
    pTree->Branch("fHitModule", &fHitModule);

    pTree->Branch("fBestTrackHitLayer", &fBestTrackHitLayer);
    pTree->Branch("fBestTrackHitXprojected", &fBestTrackHitXprojected);
    pTree->Branch("fBestTrackHitYprojected", &fBestTrackHitYprojected);
    pTree->Branch("fBestTrackHitResidU", &fBestTrackHitResidU);
    pTree->Branch("fBestTrackHitResidV", &fBestTrackHitResidV);
    pTree->Branch("fBestTrackHitUADC", &fBestTrackHitUADC);
    pTree->Branch("fBestTrackHitVADC", &fBestTrackHitVADC);
    pTree->Branch("fBestTrackHitIsampMaxUstrip", &fBestTrackHitIsampMaxUstrip);
    pTree->Branch("fBestTrackHitIsampMaxVstrip", &fBestTrackHitIsampMaxVstrip);

    // Raw GEM cluster information before tracking
    pTree -> Branch("nCluster", &nCluster, "nCluster/I");
    pTree -> Branch("planeID", &Plane);
    pTree -> Branch("prodID", &Prod);
    pTree -> Branch("moduleID", &Module);
    pTree -> Branch("axis", &Axis);
    pTree -> Branch("size", &Size);
    pTree -> Branch("adc", &Adc);
    pTree -> Branch("pos", &Pos);

    // save strip information for each cluster
    pTree -> Branch("stripNo", &StripNo);
    pTree -> Branch("stripAdc", &StripADC);

    // save apv common mode information
    pTree -> Branch("nAPV", &nAPV, "nAPV/I");
    pTree -> Branch("apv_crate_id", &apv_crate_id);
    pTree -> Branch("apv_mpd_id", &apv_mpd_id);
    pTree -> Branch("apv_adc_ch", &apv_adc_ch);
    pTree -> Branch("CM0_offline", &CM0_offline);
    pTree -> Branch("CM1_offline", &CM1_offline);
    pTree -> Branch("CM2_offline", &CM2_offline);
    pTree -> Branch("CM3_offline", &CM3_offline);
    pTree -> Branch("CM4_offline", &CM4_offline);
    pTree -> Branch("CM5_offline", &CM5_offline);
    pTree -> Branch("CM0_online", &CM0_online);
    pTree -> Branch("CM1_online", &CM1_online);
    pTree -> Branch("CM2_online", &CM2_online);
    pTree -> Branch("CM3_online", &CM3_online);
    pTree -> Branch("CM4_online", &CM4_online);
    pTree -> Branch("CM5_online", &CM5_online);

    // trigger time
    pTree -> Branch("triggerTimeL", &triggerTimeL, "triggerTimeL/I");
    pTree -> Branch("triggerTimeH", &triggerTimeH, "triggerTimeH/I");
}

GEMRootClusterTree::~GEMRootClusterTree()
{
    // place holder
}

void GEMRootClusterTree::Write()
{
    std::cout<<"Writing root cluster tree to : "<<fPath<<std::endl;
    pFile -> Write();
    pFile -> Close();
}

// a helper to get chamber based strip index, to be removed

static int getChamberBasedStripNo(int strip, int type, int N_APVS_PER_PLANE, int detLayerPositionIndex)
{
    // no conversion for Y plane
    if(type == 1)
        return strip;

    // conversion for X plane
    int c_strip = strip - N_APVS_PER_PLANE * 128 * detLayerPositionIndex;
    if(strip < 0)
    {
        std::cout<<"Error: strip conversion failed, returned without conversion."
            <<std::endl;
        return strip;
    }
    return c_strip;
}

void GEMRootClusterTree::Fill(GEMSystem *gem_sys, const uint32_t &evt_num)
{
    if(cluster_method == nullptr)
        cluster_method = new GEMCluster("config/gem_cluster.conf");

    [[maybe_unused]]int ndet = apv_strip_mapping::Mapping::Instance()->GetTotalNumberOfDetectors();

    // set event id
    evtID = static_cast<int>(evt_num);

    ClearPrevRawEvent();

    // trigger time
    std::pair<uint32_t, uint32_t> trigger_time = gem_sys -> GetTriggerTime();
    triggerTimeL = static_cast<int>(trigger_time.first);
    triggerTimeH = static_cast<int>(trigger_time.second);

    // for comon mode
    nAPV = apv_strip_mapping::Mapping::Instance() -> GetTotalNumberOfAPVs();

    // get detector list
    std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();
    for(auto &i: detectors) 
    {
        std::vector<GEMPlane*> planes = i->GetPlaneList();

        for(auto &pln: planes)
        {
            const std::vector<StripCluster> & clusters = pln -> GetStripClusters();
            int napvs_per_plane = pln -> GetCapacity();

            for(auto &c: clusters)
            {
                Plane.push_back(i -> GetLayerID());
                Prod.push_back(i -> GetDetID());
                Module.push_back(i -> GetDetLayerPositionIndex());
                Axis.push_back(static_cast<int>(pln -> GetType()));
                Size.push_back(c.hits.size());
                Adc.push_back(c.peak_charge);
                Pos.push_back(c.position);

                // strips in this cluster
                const std::vector<StripHit> &hits = c.hits;
                for(size_t nS = 0; nS < hits.size(); ++nS)
                {
                    // layer based strip no
                    //StripNo.push_back(hits[nS].strip);

                    // chamber based strip no
                    int s = getChamberBasedStripNo(hits[nS].strip, Axis.back(),
                            napvs_per_plane, Module.back());
                    StripNo.push_back(s);

                    StripADC.push_back(hits[nS].charge);
                }

                nCluster++;
            }

            // extract common mode for each apv on this plane
            std::vector<GEMAPV*> apv_list = pln -> GetAPVList();
            for(auto &apv: apv_list)
            {
                auto & online_common_mode = apv->GetOnlineCommonMode();
                auto & offline_common_mode = apv->GetOfflineCommonMode();

                apv_crate_id.push_back(apv->GetAddress().crate_id);
                apv_mpd_id.push_back(apv->GetAddress().mpd_id);
                apv_adc_ch.push_back(apv->GetAddress().adc_ch);

                if(online_common_mode.size() != 6) {
                    CM0_online.push_back(-9999);
                    CM1_online.push_back(-9999);
                    CM2_online.push_back(-9999);
                    CM3_online.push_back(-9999);
                    CM4_online.push_back(-9999);
                    CM5_online.push_back(-9999);
                } else {
                    CM0_online.push_back(online_common_mode[0]);
                    CM1_online.push_back(online_common_mode[1]); 
                    CM2_online.push_back(online_common_mode[2]); 
                    CM3_online.push_back(online_common_mode[3]); 
                    CM4_online.push_back(online_common_mode[4]); 
                    CM5_online.push_back(online_common_mode[5]); 
                }

                if(offline_common_mode.size() != 6) {
                    CM0_offline.push_back(-9999);
                    CM1_offline.push_back(-9999);
                    CM2_offline.push_back(-9999);
                    CM3_offline.push_back(-9999);
                    CM4_offline.push_back(-9999);
                    CM5_offline.push_back(-9999);
                } else {
                    CM0_offline.push_back(offline_common_mode[0]);
                    CM1_offline.push_back(offline_common_mode[1]); 
                    CM2_offline.push_back(offline_common_mode[2]); 
                    CM3_offline.push_back(offline_common_mode[3]); 
                    CM4_offline.push_back(offline_common_mode[4]); 
                    CM5_offline.push_back(offline_common_mode[5]); 
                }
            }
        }
    }

    if(nCluster > 0)
        pTree -> Fill();
}

void GEMRootClusterTree::ClearPrevTracks()
{
    besttrack = -1, fNtracks_found = 0;
    fNhitsOnTrack.clear();
    fXtrack.clear(), fYtrack.clear(), fXptrack.clear(), fYptrack.clear();
    fChi2Track.clear();

    ngoodhits = 0;
    fHitXlocal.clear(), fHitYlocal.clear(), fHitZlocal.clear();
    hit_track_index.clear();
    fHitModule.clear();

    fBestTrackHitLayer.clear();
    fBestTrackHitXprojected.clear(), fBestTrackHitYprojected.clear();
    fBestTrackHitResidU.clear(), fBestTrackHitResidV.clear();
    fBestTrackHitUADC.clear(), fBestTrackHitVADC.clear();
    fBestTrackHitIsampMaxUstrip.clear(), fBestTrackHitIsampMaxVstrip.clear();
}

void GEMRootClusterTree::ClearPrevRawEvent()
{
    nCluster = 0;
    Plane.clear();
    Prod.clear();
    Module.clear();
    Axis.clear();
    Size.clear();

    Adc.clear();
    Pos.clear();

    StripNo.clear();
    StripADC.clear();

    // for common mode study only
    nAPV = 0;
    apv_crate_id.clear();
    apv_mpd_id.clear();
    apv_adc_ch.clear();
    CM0_offline.clear();
    CM1_offline.clear();
    CM2_offline.clear();
    CM3_offline.clear();
    CM4_offline.clear();
    CM5_offline.clear();
    CM0_online.clear();
    CM1_online.clear();
    CM2_online.clear();
    CM3_online.clear();
    CM4_online.clear();
    CM5_online.clear();
}
