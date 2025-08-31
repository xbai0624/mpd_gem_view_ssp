#ifndef GEM_CLUSTER_H
#define GEM_CLUSTER_H

#include "GEMStruct.h"
#include "ConfigObject.h"

class Cuts;

class GEMCluster : public ConfigObject
{
public:
    GEMCluster(const std::string &c_path = "");
    ~GEMCluster();

    // functions that to be overloaded
    void Configure(const std::string &path = "");

    bool IsGoodStrip(const StripHit &hit) const;
    bool IsGoodCluster(const StripCluster &cluster) const;
    void FormClusters(std::vector<StripHit> &hits,
                      std::vector<StripCluster> &clusters) const;
    void CartesianReconstruct(const std::vector<StripCluster> &x_cluster,
                              const std::vector<StripCluster> &y_cluster,
                              std::vector<GEMHit> &container,
                              int det_id,
                              float resolution) const;
    void FilterClusters(std::vector<StripCluster> &clusters) const;

private:
    // private helpers
    void split_cluster(std::vector<StripHit>::iterator beg, std::vector<StripHit>::iterator end,
        double thres, std::vector<StripCluster> &clusters) const;
    void cluster_hits(std::vector<StripHit>::iterator beg, std::vector<StripHit>::iterator end,
        int con_thres, double diff_thres, std::vector<StripCluster> &clusters) const;

protected:
    void groupHits(std::vector<StripHit> &h, std::vector<StripCluster> &c) const;
    void reconstructCluster(StripCluster &cluster) const;
    void setCrossTalk(std::vector<StripCluster> &clusters) const;

protected:
    // parameters
    unsigned int min_cluster_hits;
    unsigned int max_cluster_hits;
    unsigned int consecutive_thres;
    float split_cluster_diff;
    float cross_talk_width;

    // cross talk characteristic distances
    std::vector<float> charac_dists;

    // 
    Cuts *gem_cuts;
};

#endif
