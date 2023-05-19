#ifndef GEM_ROOT_CLUSTER_TREE_H
#define GEM_ROOT_CLUSTER_TREE_H

#include <TTree.h>
#include <TFile.h>

class GEMSystem;
class GEMCluster;

////////////////////////////////////////////////////////////////////////////////
// replay evio files, and cluster all hits, save clusters to root tree

#define MAXCLUSTERS 200000
#define MAXCLUSTERSIZE 100
#define MAXAPV 1000

class GEMRootClusterTree
{
public:
    GEMRootClusterTree(const char *path);
    ~GEMRootClusterTree();

    void Write();
    void Fill(GEMSystem* gem_sys, const uint32_t &evt_num);

private:
    TTree *pTree = nullptr;
    TFile *pFile = nullptr;

    std::string fPath;

    // information to save
    int evtID;

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
    std::vector<int> fNhitsOnTrack;
    std::vector<double> fXtrack, fYtrack, fXptrack, fYptrack, fChi2Track;

    int ngoodhits; // total number of hits lies on tracks
    std::vector<double> fHitXlocal, fHitYlocal, fHitZlocal;
    std::vector<int> hit_track_index;
    std::vector<int> fHitModule;
    std::vector<int> fHitLayer;

    std::vector<double> fHitXprojected, fHitYprojected;
    std::vector<double> fHitResidU, fHitResidV;
    std::vector<double> fHitUADC, fHitVADC;
    std::vector<double> fHitIsampMaxUstrip, fHitIsampMaxVstrip;

    // -part 2):
    // Raw GEM Data
    int nCluster;            // number of clusters in current event
    int Plane[MAXCLUSTERS];  // layer id
    int Prod[MAXCLUSTERS];   // detector i
    int Module[MAXCLUSTERS]; // detector position index in layer
    int Axis[MAXCLUSTERS];   // plane x/y
    int Size[MAXCLUSTERS];   // cluster size

    double Adc[MAXCLUSTERS];  // cluster adc
    double Pos[MAXCLUSTERS];  // cluster pos

    int StripNo[MAXCLUSTERS][MAXCLUSTERSIZE];   // max 50 strips per cluster
    double StripADC[MAXCLUSTERS][MAXCLUSTERSIZE];

    // for common mode study only
    int nAPV;
    int apv_crate_id[MAXAPV];
    int apv_mpd_id[MAXAPV];
    int apv_adc_ch[MAXAPV];
    int CM0_offline[MAXAPV];
    int CM1_offline[MAXAPV];
    int CM2_offline[MAXAPV];
    int CM3_offline[MAXAPV];
    int CM4_offline[MAXAPV];
    int CM5_offline[MAXAPV];
    int CM0_online[MAXAPV];
    int CM1_online[MAXAPV];
    int CM2_online[MAXAPV];
    int CM3_online[MAXAPV];
    int CM4_online[MAXAPV];
    int CM5_online[MAXAPV];

    // event trigger time
    int triggerTimeL;
    int triggerTimeH;

    // clustering method
    GEMCluster *cluster_method = nullptr;
};

#endif
