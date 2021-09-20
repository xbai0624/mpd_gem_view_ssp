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
    pTree -> Branch("nCluster", &nCluster, "nCluster/I");
    pTree -> Branch("planeID", Plane, "planeID[nCluster]/I");
    pTree -> Branch("prodID", Prod, "prodID[nCluster]/I");
    pTree -> Branch("moduleID", Module, "moduleID[nCluster]/I");
    pTree -> Branch("axis", Axis, "axis[nCluster]/I");
    pTree -> Branch("size", Size, "size[nCluster]/I");
    pTree -> Branch("adc", Adc, "adc[nCluster]/F");
    pTree -> Branch("pos", Pos, "Pos[nCluster]/F");

    // save strip information for each cluster
    pTree -> Branch("stripNo", StripNo, "StripNo[nCluster][100]/I");
    pTree -> Branch("stripAdc", StripADC, "StripADC[nCluster][100]/F");

    // save apv common mode information
    pTree -> Branch("nAPV", &nAPV, "nAPV/I");
    pTree -> Branch("apv_crate_id", apv_crate_id, "apv_crate_id[nAPV]/I");
    pTree -> Branch("apv_mpd_id", apv_mpd_id, "apv_mpd_id[nAPV]/I");
    pTree -> Branch("apv_adc_ch", apv_adc_ch, "apv_adc_ch[nAPV]/I");
    pTree -> Branch("CM0_offline", CM0_offline, "CM0_offline[nAPV]/I");
    pTree -> Branch("CM1_offline", CM1_offline, "CM1_offline[nAPV]/I");
    pTree -> Branch("CM2_offline", CM2_offline, "CM2_offline[nAPV]/I");
    pTree -> Branch("CM3_offline", CM3_offline, "CM3_offline[nAPV]/I");
    pTree -> Branch("CM4_offline", CM4_offline, "CM4_offline[nAPV]/I");
    pTree -> Branch("CM5_offline", CM5_offline, "CM5_offline[nAPV]/I");
    pTree -> Branch("CM0_online", CM0_online, "CM0_online[nAPV]/I");
    pTree -> Branch("CM1_online", CM1_online, "CM1_online[nAPV]/I");
    pTree -> Branch("CM2_online", CM2_online, "CM2_online[nAPV]/I");
    pTree -> Branch("CM3_online", CM3_online, "CM3_online[nAPV]/I");
    pTree -> Branch("CM4_online", CM4_online, "CM4_online[nAPV]/I");
    pTree -> Branch("CM5_online", CM5_online, "CM5_online[nAPV]/I");
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
    nCluster = 0;

    // for comon mode
    nAPV = apv_strip_mapping::Mapping::Instance() -> GetTotalNumberOfAPVs();
    int apv_index = 0;

    // get detector list
    std::vector<GEMDetector*> detectors = gem_sys -> GetDetectorList();
    for(auto &i: detectors) 
    {
        std::vector<GEMPlane*> planes = i->GetPlaneList();

        for(auto &pln: planes) 
        {
            const std::vector<StripCluster> & clusters = pln -> GetStripClusters();
            int napvs_per_plane = pln -> GetCapacity();
            for(auto &c: clusters) {
                Plane[nCluster] = i -> GetLayerID();
                Prod[nCluster] = i -> GetDetID();
                Module[nCluster] = i -> GetDetLayerPositionIndex();
                Axis[nCluster] = static_cast<int>(pln -> GetType());
                Size[nCluster] = c.hits.size();
                Adc[nCluster] = c.peak_charge;
                Pos[nCluster] = c.position;

                // strips in this cluster
                const std::vector<StripHit> &hits = c.hits;
                for(size_t nS = 0; nS < hits.size() && nS < 100; ++nS)
                {
                    // layer based strip no
                    //StripNo[nCluster][nS] = hits[nS].strip;

                    // chamber based strip no
                    StripNo[nCluster][nS] = getChamberBasedStripNo(hits[nS].strip, Axis[nCluster],
                           napvs_per_plane, Module[nCluster]);
 
                    StripADC[nCluster][nS] = hits[nS].charge;
                }

                nCluster++;
                if(nCluster >= MAXCLUSTERS)
                    break;
            }

            // extract common mode for each apv on this plane
            std::vector<GEMAPV*> apv_list = pln -> GetAPVList();
            for(auto &apv: apv_list)
            {
                auto & online_common_mode = apv->GetOnlineCommonMode();
                auto & offline_common_mode = apv->GetOfflineCommonMode();

                apv_crate_id[apv_index] = apv->GetAddress().crate_id;
                apv_mpd_id[apv_index] = apv->GetAddress().mpd_id;
                apv_adc_ch[apv_index] = apv->GetAddress().adc_ch;

                if(online_common_mode.size() != 6) {
                    CM0_online[apv_index] = -9999;
                    CM1_online[apv_index] = -9999;
                    CM2_online[apv_index] = -9999;
                    CM3_online[apv_index] = -9999;
                    CM4_online[apv_index] = -9999;
                    CM5_online[apv_index] = -9999;
                } else {
                    CM0_online[apv_index] = online_common_mode[0];
                    CM1_online[apv_index] = online_common_mode[1]; 
                    CM2_online[apv_index] = online_common_mode[2]; 
                    CM3_online[apv_index] = online_common_mode[3]; 
                    CM4_online[apv_index] = online_common_mode[4]; 
                    CM5_online[apv_index] = online_common_mode[5]; 
                }

                if(offline_common_mode.size() != 6) {
                    CM0_offline[apv_index] = -9999;
                    CM1_offline[apv_index] = -9999;
                    CM2_offline[apv_index] = -9999;
                    CM3_offline[apv_index] = -9999;
                    CM4_offline[apv_index] = -9999;
                    CM5_offline[apv_index] = -9999;
                } else {
                    CM0_offline[apv_index] = offline_common_mode[0];
                    CM1_offline[apv_index] = offline_common_mode[1]; 
                    CM2_offline[apv_index] = offline_common_mode[2]; 
                    CM3_offline[apv_index] = offline_common_mode[3]; 
                    CM4_offline[apv_index] = offline_common_mode[4]; 
                    CM5_offline[apv_index] = offline_common_mode[5]; 
                }

                apv_index++;
            }
        }
    }

    if(nCluster > 0)
        pTree -> Fill();
}
