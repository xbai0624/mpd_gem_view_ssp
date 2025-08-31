#ifndef GEM_ROOT_CLUSTER_TREE_H
#define GEM_ROOT_CLUSTER_TREE_H

#include <TTree.h>
#include <TFile.h>

class GEMSystem;
class GEMCluster;

////////////////////////////////////////////////////////////////////////////////
// replay evio files, and cluster all hits, save clusters to root tree

//#define MAXCLUSTERS 200000
//#define MAXCLUSTERSIZE 100
//#define MAXAPV 1000

class GEMRootClusterTree
{
public:
    GEMRootClusterTree(const char *path);
    ~GEMRootClusterTree();

    void Write();
    void Fill(GEMSystem* gem_sys, const uint32_t &evt_num);

    void ClearPrevTracks();
    void ClearPrevRawEvent();

private:
    TTree *pTree = nullptr;
    TFile *pFile = nullptr;

    std::string fPath;

    // information to save
    int evtID;

public:
    // -part 1):
    // tracking result
    // tracking result saves all possible tracks that pass chi2 cut
    //   besttrack - variable that keeps the track index with minimum chi2
    //   fNtracks_found - saves the total number of tracks found
    //   fNhitsOntrack - saves how many hits on each track, similar for fXtrack ...
    //
    //   ngoodhits - saves the total number of hits that lies on tracks
    //       this is not exclusive, for example: one hit can lie on two possible tracks
    //       in this case, the hit will be duplicated/copied, one for each track, and this
    //       number will reflect that
    //   fHitXlocal - , fHitYlocal - , ... etc - saves all hits that lies on tracks, similar
    //       to ngoodhits, they are not exclusive, if a hit lies on two possible
    //       tracks, it will be copied twice, if three tracks, then copied three times
    //       this is to be compatible with SBS tree organization
    //   hit_track_index - saves the track index for this hit
    //   fHitModule - saves the module id for this hit
    //
    //   fHitLayer - only applies for best track, the hit layer id for hits on the best track
    //       similar for fHitXprojected, ..., and the rest
    int besttrack;
    int fNtracks_found;
    int fNAllGoodTrackCandidates;
    std::vector<int> fNhitsOnTrack;
    std::vector<double> fXtrack, fYtrack, fXptrack, fYptrack, fChi2Track;

    int ngoodhits; // total number of hits lies on tracks
    std::vector<double> fHitXlocal, fHitYlocal, fHitZlocal;
    std::vector<int> hit_track_index;
    std::vector<int> fHitModule;

    // for best track
    std::vector<int> fBestTrackHitLayer;
    std::vector<double> fBestTrackHitXprojected, fBestTrackHitYprojected;
    std::vector<double> fBestTrackHitResidU, fBestTrackHitResidV;
    std::vector<double> fBestTrackHitUADC, fBestTrackHitVADC;
    std::vector<double> fBestTrackHitIsampMaxUstrip, fBestTrackHitIsampMaxVstrip;

private:
    // -part 2):
    // Raw GEM Data
    int nCluster;            // number of clusters in current event
    std::vector<int> Plane;  // layer id
    std::vector<int> Prod;   // detector i
    std::vector<int> Module; // detector position index in layer
    std::vector<int> Axis;   // plane x/y
    std::vector<int> Size;   // cluster size

    std::vector<double> Adc;  // cluster adc
    std::vector<double> Pos;  // cluster pos

    // stripNo and stripADC are pushed into the vectors sequentially
    // following the order of clusters, so to find all strips for a 
    // specific cluster, one needs to do the following:
    //
    // int strip_counter = 0;
    // for(int i_cluster =0; i_cluster<nCluster; i_cluster++)
    // {
    //     for(int i_strip=0, i_strip<Size[i_cluster]; i_strip++) {
    //         int stripNo = StripNo[strip_counter + i_strip];
    //         double stripAdc = StripADC[strip_counter + i_strip];
    //     }
    //
    //     strip_counter += Size[i_cluster];
    // }
    std::vector<int> StripNo;
    std::vector<double> StripADC;

    // for common mode study only
    int nAPV;
    std::vector<int> apv_crate_id;
    std::vector<int> apv_mpd_id;
    std::vector<int> apv_adc_ch;
    std::vector<int> CM0_offline;
    std::vector<int> CM1_offline;
    std::vector<int> CM2_offline;
    std::vector<int> CM3_offline;
    std::vector<int> CM4_offline;
    std::vector<int> CM5_offline;
    std::vector<int> CM0_online;
    std::vector<int> CM1_online;
    std::vector<int> CM2_online;
    std::vector<int> CM3_online;
    std::vector<int> CM4_online;
    std::vector<int> CM5_online;

    // event trigger time
    int triggerTimeL;
    int triggerTimeH;

    // clustering method
    GEMCluster *cluster_method = nullptr;
};

#endif
