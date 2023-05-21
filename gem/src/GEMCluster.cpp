//============================================================================//
// Basic Cluster Reconstruction Class For GEM                                 //
// GEM Planes send hits infromation and container for the GEM Clusters to this//
// class reconstruct the clusters and send it back to GEM Planes.             //
// Thus the clustering algorithm can be adjusted in this class.               //
//                                                                            //
// Xinzhan Bai & Kondo Gnanvo, first version coding of the algorithm          //
// Chao Peng,                                                                 //
// 05/04/2016                                                                 //
// Xinzhan Bai, add cross talk removal                                        //
// 10/21/2016                                                                 //
// Xinzhan Bai, adapted to MPD system                                         //
// 02/09/2021                                                                 //
//============================================================================//


#include "GEMCluster.h"
#include "Cuts.h"
#include <algorithm>

#define USE_GEM_CUT

////////////////////////////////////////////////////////////////////////////////
// ctor

GEMCluster::GEMCluster(const std::string &config_path)
{
    Configure(config_path);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

GEMCluster::~GEMCluster()
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// configure

void GEMCluster::Configure([[maybe_unused]]const std::string &path)
{
    // if no configuration file specified, load the default value quietly
    bool verbose = false;

    if(!path.empty()) {
        ConfigObject::Configure(path);
        verbose = true;
    }

    CONF_CONN(min_cluster_hits, "Min Cluster Hits", 1, verbose);
    CONF_CONN(max_cluster_hits, "Max Cluster Hits", 20, verbose);
    CONF_CONN(split_cluster_diff, "Split Threshold", 14, verbose);
    CONF_CONN(cross_talk_width, "Cross Talk Width", 2, verbose);
    CONF_CONN(consecutive_thres, "Consecutive Threshold", 1, verbose);

    // get cross talk characteristic distance
    charac_dists.clear();
    std::string dist_str = Value<std::string>("Characteristic Distance");
    charac_dists = ConfigParser::stofs(dist_str, ",", " \t");

    gem_cuts = new Cuts();
    //gem_cuts -> Print();

    min_cluster_hits = gem_cuts -> __get("min cluster size").val<int>();
    max_cluster_hits = gem_cuts -> __get("max cluster size").val<int>();
}

////////////////////////////////////////////////////////////////////////////////
// group hits into clusters

void GEMCluster::FormClusters([[maybe_unused]]std::vector<StripHit> &hits,
                              [[maybe_unused]]std::vector<StripCluster> &clusters) 
const
{
    // clean container first
    clusters.clear();

    // group consecutive hits as the preliminary clusters
    groupHits(hits, clusters);

    // reconstruct the cluster position
    for(auto &cluster : clusters)
    {
        reconstructCluster(cluster);
    }

    // set cross talk flag
    //setCrossTalk(clusters); // xinzhan: debug: temporarily disable cross talk removal

    // filter clusters
    FilterClusters(clusters);
}

////////////////////////////////////////////////////////////////////////////////
// is it a good strip

bool GEMCluster::IsGoodStrip(const StripHit &hit) const
{
#ifdef USE_GEM_CUT
    // time bin cut
    if(!gem_cuts -> max_time_bin(hit))
        return false;

    // strip avg time cut
    if(!gem_cuts -> strip_mean_time(hit))
        return false;
#endif
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// a helper function to further separate hits at minimum

//template<class Iter>
//inline void split_cluster(Iter beg, Iter end, double thres, std::vector<StripCluster> &clusters)
void GEMCluster::split_cluster(std::vector<StripHit>::iterator beg, std::vector<StripHit>::iterator end,
        double thres, std::vector<StripCluster> &clusters) const
{
    auto size = end - beg;
    if(size <= 0)
        return;

    // disable cluster split when cluster size < 3
    if(size < 3) {
        clusters.emplace_back(std::vector<StripHit>(beg, end));
        return;
    }

    // find the first local minimum
    bool descending = false, extremum = false;
    auto minimum = beg;
    for(auto it = beg, it_n = beg + 1; it_n != end; ++it, ++it_n)
    {
        if(descending) {
            // update minimum
            if(it->charge < minimum->charge)
                minimum = it;

            // transcending trend, confirm a local minimum (valley)
            if(it_n->charge - it->charge > thres) {
                extremum = true;
                // only needs the first local minimum, thus exit the loop
                break;
            }
        } else {
            // descending trend, expect a local minimum
            if(it->charge - it_n->charge > thres) {
                descending = true;
                minimum = it_n;
            }
        }
    }

    if(extremum) {
        // half the charge of overlap strip
        minimum->charge /= 2.;

        // new split cluster
        clusters.emplace_back(std::vector<StripHit>(beg, minimum));

        // check the leftover strips
        split_cluster(minimum, end, thres, clusters);
    } else {
        clusters.emplace_back(std::vector<StripHit>(beg, end));
    }
}

////////////////////////////////////////////////////////////////////////////////
// cluster consecutive hits

//template<class Iter>
//inline void cluster_hits(Iter beg, Iter end, int con_thres, double diff_thres, std::vector<StripCluster> &clusters)
void GEMCluster::cluster_hits(std::vector<StripHit>::iterator beg, std::vector<StripHit>::iterator end,
        int con_thres, double diff_thres, std::vector<StripCluster> &clusters) const
{
    auto cbeg = beg;
    for(auto it = beg; it != end; ++it)
    {
        if(!IsGoodStrip(*it))
        {
            if(cbeg != it) {
                split_cluster(cbeg, it, diff_thres, clusters);
            }
            cbeg = it+1;
            continue;
        }

        auto it_n = it + 1;
        if((it_n == end) || (it_n->strip - it->strip > con_thres)) {
            split_cluster(cbeg, it_n, diff_thres, clusters);
            cbeg = it_n;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// group consecutive hits

void GEMCluster::groupHits(std::vector<StripHit> &hits, std::vector<StripCluster> &clusters)
const
{
    // sort hits by its strip number
    std::sort(hits.begin(), hits.end(),
              // lambda expr, compare hit by their strip numbers
              [](const StripHit &h1, const StripHit &h2)
              {
                  return h1.strip < h2.strip;
              });

    // cluster hits
    cluster_hits(hits.begin(), hits.end(), consecutive_thres, split_cluster_diff, clusters);
}

////////////////////////////////////////////////////////////////////////////////
// helper function to check cross talk strips

inline bool is_pure_ct(const StripCluster &cl)
{
    for(auto &hit : cl.hits)
    {
        // still has non-cross-talk strips
        if(!hit.cross_talk)
            return false;
    }

    // pure cross talk strips
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// helper function to check cross talk characteristic distance

typedef std::vector<StripCluster>::iterator SCit;
inline bool ct_distance(SCit it, SCit end, float width, const std::vector<float> &charac)
{
    if(it == end)
        return false;

    for(auto itn = it + 1; itn != end; ++itn)
    {
        float delta = std::abs(it->position - itn->position);

        for(auto &dist : charac)
        {
            if((delta > dist - width) && (delta < dist + width))
                return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// 

void GEMCluster::setCrossTalk(std::vector<StripCluster> &clusters)
const
{
    // sort by peak charge
    std::sort(clusters.begin(), clusters.end(),
              [](const StripCluster &c1, const StripCluster &c2)
              {
                  return c1.peak_charge < c2.peak_charge;
              });

    for(auto it = clusters.begin(); it != clusters.end(); ++it)
    {
        // only remove cross talk clusters that is a composite of cross talk strips
        if(!is_pure_ct(*it))
            continue;

        it->cross_talk = ct_distance(it, clusters.end(), cross_talk_width, charac_dists);
    }
}

////////////////////////////////////////////////////////////////////////////////
// calculate the cluster position
// it reconstruct the position of cluster using linear weight of charge fraction

void GEMCluster::reconstructCluster(StripCluster &cluster)
const
{
    // no hits
    if(cluster.hits.empty())
        return;

    // determine position, peak charge and total charge of the cluster
    cluster.total_charge = 0.;
    cluster.peak_charge = 0.;
    cluster.max_timebin = -1;
    float weight_pos = 0.;

    short index = 0;
    for(auto &hit : cluster.hits)
    {
        if(cluster.peak_charge < hit.charge) {
            cluster.peak_charge = hit.charge;
            cluster.max_timebin = hit.max_timebin;
        }

        cluster.total_charge += hit.charge;
        weight_pos +=  hit.position*hit.charge;

        index++;
    }

    cluster.position = weight_pos/cluster.total_charge;
}


////////////////////////////////////////////////////////////////////////////////
// is it a good cluster

bool GEMCluster::IsGoodCluster([[maybe_unused]]const StripCluster &cluster) const
{
#ifdef USE_GEM_CUT
    // bad size
    if((cluster.hits.size() < min_cluster_hits) ||
       (cluster.hits.size() > max_cluster_hits))
        return false;

    if(!(gem_cuts -> seed_strip_min_peak_adc(cluster)))
        return false;

    if(!(gem_cuts -> seed_strip_min_sum_adc(cluster)))
        return false;

    if(!(gem_cuts -> cluster_strip_time_agreement(cluster)))
        return false;
#endif
    // not a cross talk cluster
    return !cluster.cross_talk;
}


////////////////////////////////////////////////////////////////////////////////
// filter clusters

void GEMCluster::FilterClusters(std::vector<StripCluster> &clusters) 
const 
{
    // check mapping
    [[maybe_unused]] auto belongs_to_apv = [](int crate, int mpd, int ch, const StripCluster &c) -> bool
    {
        APVAddress addr(crate, mpd, ch);

        for(auto &i: c.hits) {
            if(i.apv_addr == addr)
                return true;
        }

        return false;
    };

    // TODO, probably add some criteria here to filter out some bad clusters
    std::vector<StripCluster> temp_clusters;
    auto it = clusters.begin();
    for(; it != clusters.end(); it++)
    {
        if(!IsGoodCluster(*it))
            continue;

        temp_clusters.push_back(*it);
    }

    clusters.clear();
    clusters = temp_clusters;
}

////////////////////////////////////////////////////////////////////////////////
// this function accepts x, y clusters from detectors and then form GEM Cluster
// it return the number of clusters

void GEMCluster::CartesianReconstruct([[maybe_unused]]const std::vector<StripCluster> &x_cluster,
                                      [[maybe_unused]]const std::vector<StripCluster> &y_cluster,
                                      [[maybe_unused]]std::vector<GEMHit> &container,
                                      [[maybe_unused]]int det_id,
                                      [[maybe_unused]]float resolution) 
const
{
    // empty first
    container.clear();

    // fill possible clusters in
    for(auto &xc : x_cluster)
    {
        for(auto &yc : y_cluster)
        {
#ifdef USE_GEM_CUT
            if(!(gem_cuts -> cluster_adc_assymetry(xc, yc)))
                continue;

            if(!(gem_cuts -> cluster_time_assymetry(xc, yc)))
                continue;
#endif
            container.emplace_back(xc.position, yc.position, 0.,        // by default z = 0
                                   det_id,                              // detector id
                                   xc.total_charge, yc.total_charge,    // fill in total charge
                                   xc.peak_charge, yc.peak_charge,      // fill in peak charge
                                   xc.max_timebin, yc.max_timebin,      // fill in the max time bin
                                   xc.hits.size(), yc.hits.size(),      // number of hits
                                   resolution);                         // position resolution
        }
    }
}
