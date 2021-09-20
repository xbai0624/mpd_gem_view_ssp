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
    int nCluster;            // number of clusters in current event
    int Plane[MAXCLUSTERS];  // layer id
    int Prod[MAXCLUSTERS];   // detector i
    int Module[MAXCLUSTERS]; // detector position index in layer
    int Axis[MAXCLUSTERS];   // plane x/y
    int Size[MAXCLUSTERS];   // cluster size

    float Adc[MAXCLUSTERS];  // cluster adc
    float Pos[MAXCLUSTERS];  // cluster pos

    int StripNo[MAXCLUSTERS][MAXCLUSTERSIZE];   // max 50 strips per cluster
    float StripADC[MAXCLUSTERS][MAXCLUSTERSIZE];

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

    // clustering method
    GEMCluster *cluster_method = nullptr;
};

#endif
