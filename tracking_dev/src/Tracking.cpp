#include "Tracking.h"
#include "TrackingUtility.h"
#include "AbstractDetector.h"
#include <iostream>
#include <algorithm>

namespace tracking_dev {

#define USE_GRID

Tracking::Tracking()
{
    tracking_utility = new TrackingUtility();
    tracking_cuts = new Cuts();
}

Tracking::~Tracking()
{
}

void Tracking::AddDetector(int index, AbstractDetector* det)
{
    if(detector.find(index) != detector.end())
    {
        std::cout<<"ERROR: duplicated detector index added to tracking."
            <<std::endl;
        exit(0);
    }

    layer_index.push_back(index);
    detector[index] = det;
}

void Tracking::CompleteSetup()
{
    minimum_hits_on_track = (tracking_cuts -> __get("minimum hits on track")).val<int>();
    chi2_cut = (tracking_cuts -> __get("track max chi2")).val<float>();
    abort_quantity = (tracking_cuts -> __get("abort tracking quantity")).val<int>();
    max_track_save_quantity = (tracking_cuts -> __get("save max track quantity")).val<int>();

    k_min_xz = (tracking_cuts -> __get("track x-z slope range")).arr<double>()[0];
    k_max_xz = (tracking_cuts -> __get("track x-z slope range")).arr<double>()[1];
    k_min_yz = (tracking_cuts -> __get("track y-z slope range")).arr<double>()[0];
    k_max_yz = (tracking_cuts -> __get("track y-z slope range")).arr<double>()[1];

    initLayerGroups();
    //PrintLayerGroups();
   
    std::cout<<"INFO:: Tracking setup completed."<<std::endl;
}

void Tracking::FindTracks()
{
    ClearPreviousEvent();

    //PrintHitStatus();

    loopAllLayerGroups();

    vectorize_map();

    //std::cout<<"---- best hits used to fit tracks: "<<std::endl;
    //for(auto &i: best_hits_on_track)
    //    std::cout<<i;
    //std::cout<<std::endl;
    //std::cout<<"best track chi2ndf = "<<best_track_chi2ndf<<std::endl;
    //std::cout<<"best layer combination: "<<std::endl;
    //Print(best_track_layer_index);
    //std::cout<<"best hit combination: "<<std::endl;
    //Print(best_track_hit_index);
    //for(unsigned int i=0; i<best_track_layer_index.size(); i++)
    //{
    //    int layerid = best_track_layer_index[i];
    //    int hitid = best_track_hit_index[i];
    //    std::cout<<"layer: "<<layerid<<", hit id: "<<hitid<<std::endl;
    //    std::cout<<detector[layerid]->Get2DHit(hitid);
    //}
}

void Tracking::ClearPreviousEvent()
{
    best_track_index = -1;
    best_track_chi2ndf = LARGE_VALUE;
    best_xtrack = LARGE_VALUE; best_ytrack = LARGE_VALUE;
    best_xptrack = LARGE_VALUE; best_yptrack = LARGE_VALUE;
    nhits_on_best_track = LARGE_VALUE;

    // optional
    best_track_layer_index.clear();
    best_track_hit_index.clear();

    //
    best_track_chi2ndf_by_nlayer.clear();

    // debug
    best_hits_on_track.clear();

    n_good_track_candidates =  0;
    n_tracks_found = 0;
    v_xtrack.clear(), v_ytrack.clear(), v_xptrack.clear(), v_yptrack.clear();
    v_track_chi2ndf.clear();
    v_track_nhits.clear();

    n_total_good_hits = 0;
    v_xlocal.clear(), v_ylocal.clear(), v_zlocal.clear();
    v_hit_track_index.clear();
    v_hit_module.clear();

    m_xtrack.clear(), m_ytrack.clear(), m_xptrack.clear(), m_yptrack.clear();
    m_track_chi2ndf.clear();
    m_track_nhits.clear();
    m_xlocal.clear(), m_ylocal.clear(), m_zlocal.clear();
    m_hit_track_index.clear();
    m_hit_module.clear();
}

bool Tracking::GetBestTrack(double &xt, double &yt, double &xp, double &yp, double &chi)
{
    if(best_xtrack >= LARGE_VALUE)
        return false;

    xt = best_xtrack; yt = best_ytrack;
    xp = best_xptrack; yp = best_yptrack;
    chi = best_track_chi2ndf;

    return true;
}

void Tracking::initHitStatus()
{
    hit_used.clear();

    for(auto &i: detector)
    {
        unsigned int n = i.second -> Get2DHitCounts();
        std::vector<bool> tmp(n, false);
        hit_used[i.first] = tmp;
    }
}

// this algorithm favors tracks with more layers
void Tracking::loopAllLayerGroups()
{
    int nlayers = (int)layer_index.size();

    while(nlayers >= minimum_hits_on_track)
    {
        for(auto &i: group_nlayer[nlayers])
        {
#ifdef USE_GRID
            nextLayerGroup_gridway(i);
#else
            nextLayerGroup(i);
#endif  
        }

        // if we found a track with higher number of layers,
        // then there's no need to continue search with less layer configurations
        if(found_tracks_with_nlayer(nlayers))
            break;

        nlayers--;
    }
}

//
void Tracking::nextLayerGroup(const std::vector<int> &group)
{
    std::vector<int> hit_counts;

    for(auto &i: group)
    {
        unsigned int nhits_this_layer = detector[i] -> Get2DHitCounts();
        hit_counts.push_back((int) nhits_this_layer);
    }

    //std::cout<<"-------------- layer index: "<<std::endl;
    //Print(group);
    //std::cout<<"-------------- hit counts in layers: "<<std::endl;
    //Print(hit_counts);

    std::vector<int> hit_comb;
    scanCandidate(hit_counts, group, hit_comb);
}

//
void Tracking::scanCandidate(const std::vector<int>& nhit_by_layer,
        const std::vector<int> &layer_id,
        std::vector<int> &hit_comb)
{
    if(hit_comb.size() == nhit_by_layer.size())
    {
        current_hit_comb.clear();
        current_hit_comb = hit_comb;
        current_layer_comb.clear();
        current_layer_comb = layer_id; // cache current layer group
 
        //std::cout<<"hit combo: "<<std::endl;
        //Print(hit_comb);
        nextTrackCandidate(layer_id, hit_comb);

        return;
    }

    if(hit_comb.size() == nhit_by_layer.size() - 1)
    {
        for(int i=0; i<nhit_by_layer.back(); i++)
        {
            hit_comb.push_back(i);
            scanCandidate(nhit_by_layer, layer_id, hit_comb);
            hit_comb.pop_back();
        }
    }
    else
    {
        for(int i=0; i<nhit_by_layer[hit_comb.size()]; i++)
        {
            hit_comb.push_back(i);
            scanCandidate(nhit_by_layer, layer_id, hit_comb);
            hit_comb.pop_back();
        }
    }
}

// using grid method to search hits combnations
void Tracking::nextLayerGroup_gridway(const std::vector<int> &group)
{
    // outter layers
    int start_layer = group[0];
    int end_layer = group.back();

    // middle layers
    std::vector<int> middle_layers;
    for(int i=1; i<(int)group.size() - 1; i++)
    {
        middle_layers.push_back(group[i]);
    }

    // optics cut for outer layers - to be implemented in here
    int S = (int)detector.at(start_layer) -> Get2DHitCounts();
    int E = (int)detector.at(end_layer) -> Get2DHitCounts();

    // if possible combinations in outter layers already passed max quantity, abort tracking
    if(S * E > abort_quantity) return;

    for(int start_layer_hit_index=0; start_layer_hit_index<S; start_layer_hit_index++)
    {
        for(int end_layer_hit_index=0; end_layer_hit_index<E; end_layer_hit_index++)
        {
            scanCandidate_gridway(start_layer, start_layer_hit_index,
                    end_layer, end_layer_hit_index, middle_layers);
        }
    }
}

// a helper
void Tracking::scanCandidate_gridway(const int &start_layer, const int &start_layer_hit_index,
        const int &end_layer, const int& end_layer_hit_index,
        const std::vector<int> &middle_layers)
{
    std::unordered_map<int, std::vector<int>> hit_index_by_layer; // for middle layers

    getMiddleLayerGridHitIndex(start_layer, start_layer_hit_index,
            end_layer, end_layer_hit_index, middle_layers, hit_index_by_layer);

    // abort tracking when combinations is too many, too much computing time
    int possible_track_combinations = ((int)detector.at(start_layer) -> Get2DHitCounts()) *
        ((int)detector.at(end_layer) -> Get2DHitCounts());
    for(auto &i: middle_layers)
        possible_track_combinations *= (hit_index_by_layer.at(i).size());

    if(possible_track_combinations > abort_quantity)
        return;

    std::vector<int> layer_combo{start_layer, end_layer};
    std::vector<int> hit_combo{start_layer_hit_index, end_layer_hit_index};

    int remaining_layers = middle_layers.size();
    scanCandidate_gridway(hit_index_by_layer, layer_combo, hit_combo, 
            middle_layers, remaining_layers);
}

// a recursive helper
void Tracking::scanCandidate_gridway(const std::unordered_map<int, std::vector<int>> &vhitid_by_layer,
        std::vector<int> layer_combo, std::vector<int> hit_combo,
        const std::vector<int> &middle_layers,
        int remainning_layer)
{
    if(remainning_layer < 0)
        return;

    if(remainning_layer == 0)
    {
        // found candidates
        current_hit_comb.clear();
        current_hit_comb = hit_combo;
        current_layer_comb.clear();
        current_layer_comb = layer_combo; // cache current layer group

        nextTrackCandidate(layer_combo, hit_combo);
        return;
    }

    int layer = middle_layers.at(remainning_layer-1);
    remainning_layer--;
    layer_combo.push_back(layer);

    for(auto &i: vhitid_by_layer.at(layer))
    {
        hit_combo.push_back(i);

        scanCandidate_gridway(vhitid_by_layer, layer_combo, hit_combo,
                middle_layers, remainning_layer);

        hit_combo.pop_back();
    }
}

// a helper
void Tracking::getMiddleLayerGridHitIndex(const int &start_layer, 
        const int &start_layer_hitindex, const int &end_layer, const int &end_layer_hitindex,
        const std::vector<int> &middle_layers,
        std::unordered_map<int, std::vector<int>> &hit_index_by_layer)
{
    point_t p_start = detector[start_layer] -> Get2DHit(start_layer_hitindex);
    point_t p_end = detector[end_layer] -> Get2DHit(end_layer_hitindex);

    for(auto &i: middle_layers)
    {
        double z = detector[i] ->GetZPosition();
        point_t p = tracking_utility -> intersection_point(p_start, p_end, z);
        std::vector<grid_addr_t> home_grids = detector[i] -> GetPointHomeGrids(p);

        const auto &vhits_by_grid = detector[i] -> GetGridVHits();

        std::vector<int> tmp_vhits;

        for(auto &addr: home_grids) {
            if(vhits_by_grid.find(addr) == vhits_by_grid.end())
                continue;

            tmp_vhits.insert(tmp_vhits.end(), vhits_by_grid.at(addr).begin(),
                    vhits_by_grid.at(addr).end());
        }

        hit_index_by_layer[i] = tmp_vhits;
    }
}

// a helper for the recursive call, to help getCombinationList() function
static void scan_layer_combination(
        const std::vector<int> &l, // to select from
        std::vector<int> member,   // a cache
        unsigned int pos,          // current position
        const unsigned int &n,     // required size
        std::vector<std::vector<int>> &r)
{
    if(pos >= l.size()) return;

    if(member.size() == n-1) {
        for(unsigned int i=pos; i<l.size(); i++)
        {
            std::vector<int> tmp(member);
            tmp.push_back(l[i]);
            r.push_back(tmp);
        }
    }
    else {
        for(unsigned int i=pos; i<l.size(); i++)
        {
            member.push_back(l[i]);
            scan_layer_combination(l, member, i+1, n, r);
            member.pop_back();
        }
    }
}

//
void Tracking::getCombinationList(const std::vector<int> &layers, const int &m,
        std::vector<std::vector<int>> &res)
{
    if((int)layers.size() < m)
        return;

    res.clear();

    unsigned int pos = 0;
    std::vector<int> member;

    scan_layer_combination(layers, member, pos, m, res);
}

//
void Tracking::nextTrackCandidate(const std::vector<int> &layer_id,
        const std::vector<int> &hit_index)
{
    if(layer_id.size() != hit_index.size()) {
        std::cout<<"ERROR: layer index and hit index not in same length."<<std::endl;
        exit(0);
    }

    std::vector<point_t> hits;

    for(unsigned int i=0; i<layer_id.size(); i++)
    {
        hits.push_back(detector[layer_id[i]] -> Get2DHit(hit_index[i]));
    }

    nextTrackCandidate(hits);
}

//
void Tracking::nextTrackCandidate(const std::vector<std::pair<int, int>>& combo)
{
    std::vector<point_t> hits;
    for(auto &i: combo) {
        hits.push_back(detector[i.first] -> Get2DHit(i.second));
    }

    nextTrackCandidate(hits);
}

//
void Tracking::nextTrackCandidate(const std::vector<point_t> &hits)
{
    // @parameters:
    //           (xtrack, ytrack) : track projected 2D points at z = 0
    //         (xptrack, yptrack) : track slope at x-z, y-z plane
    //                    chi2ndf : reduced chi square
    // std::vector<double> xresid : x residue for each layer
    // std::vector<double> yresid : y residue for each layer

    double xtrack, ytrack, xptrack, yptrack, chi2ndf;
    std::vector<double> xresid, yresid;

    tracking_utility -> FitLine(hits, xtrack, ytrack, xptrack, yptrack,
            chi2ndf, xresid, yresid);

    // slope cut
    if(xptrack < k_min_xz || xptrack > k_max_xz) return;
    if(yptrack < k_min_yz || yptrack > k_max_yz) return;

    // chi2ndf too big
    if(chi2ndf > chi2_cut) return;

    // using map here is only for sorting purpose, keep the 20 lowest chi2 tracks
    if((int)m_xtrack.size() <= max_track_save_quantity || chi2ndf < (std::prev(m_xtrack.end()) -> first))
    {
        n_good_track_candidates++;
        n_tracks_found++;
        m_xtrack[chi2ndf] = xtrack, m_ytrack[chi2ndf] = ytrack;
        m_xptrack[chi2ndf] = xptrack, m_yptrack[chi2ndf] = yptrack;
        m_track_chi2ndf[chi2ndf] = chi2ndf; // for consistency
        m_track_nhits[chi2ndf] = (int)hits.size();

        int track_index_ = std::distance(m_xtrack.begin(), m_xtrack.find(chi2ndf));

        n_total_good_hits += (int)hits.size();
        for(auto &i: hits) {
            m_xlocal[chi2ndf].emplace_back(i.x), m_ylocal[chi2ndf].emplace_back(i.y);
            m_zlocal[chi2ndf].emplace_back(i.z);
            m_hit_track_index[chi2ndf].emplace_back(track_index_);
            m_hit_module[chi2ndf].emplace_back(i.module_id);
        }
    }

    // erase the current biggest chi2 track
    if((int)m_xtrack.size() > max_track_save_quantity)
    {
        n_tracks_found--;
        m_xtrack.erase(std::prev(m_xtrack.end())), m_ytrack.erase(std::prev(m_ytrack.end()));
        m_xptrack.erase(std::prev(m_xptrack.end())), m_yptrack.erase(std::prev(m_yptrack.end()));
        m_track_chi2ndf.erase(std::prev(m_track_chi2ndf.end()));
        m_track_nhits.erase(std::prev(m_track_nhits.end()));

        int nhits_in_last_track = m_xlocal.rbegin() -> second.size();
        n_total_good_hits -= nhits_in_last_track;
        m_xlocal.erase(std::prev(m_xlocal.end())), m_ylocal.erase(std::prev(m_ylocal.end()));
        m_zlocal.erase(std::prev(m_zlocal.end()));
        m_hit_track_index.erase(std::prev(m_hit_track_index.end()));
        m_hit_module.erase(std::prev(m_hit_module.end()));
    }

    // best track, the one with minimum chi2
    if(chi2ndf < best_track_chi2ndf)
    {
        best_track_index = (int)v_xtrack.size() - 1;

        best_track_chi2ndf = chi2ndf;
        best_xtrack = xtrack;
        best_ytrack = ytrack;
        best_xptrack = xptrack;
        best_yptrack = yptrack;

        // optional
        best_track_layer_index = current_layer_comb;
        best_track_hit_index = current_hit_comb;

        nhits_on_best_track = (int) hits.size();

        // chi2ndf by number of layers
        best_track_chi2ndf_by_nlayer[nhits_on_best_track] = chi2ndf;

        // debug
        best_hits_on_track.clear();
        best_hits_on_track = hits;
    }
}

//
void Tracking::initLayerGroups()
{
    for(unsigned int i=minimum_hits_on_track; i<=layer_index.size(); i++)
    {
        std::vector<std::vector<int>> res;
        getCombinationList(layer_index, i, res);

        group_nlayer[i] = res;

        //best_track_chi2ndf_by_nlayer[i] = LARGE_VALUE;
        best_track_chi2ndf_by_nlayer.clear();
    }
}

// 
bool Tracking::found_tracks_with_nlayer(int nlayer)
{
    if(best_track_chi2ndf_by_nlayer.find(nlayer) == best_track_chi2ndf_by_nlayer.end())
        return false;

    if(best_track_chi2ndf_by_nlayer.at(nlayer) > chi2_cut)
        return false;

    return true;
}

// 
void Tracking::vectorize_map()
{
    vectorize_map<double>(m_xtrack, v_xtrack), vectorize_map<double>(m_ytrack, v_ytrack);
    vectorize_map<double>(m_xptrack, v_xptrack), vectorize_map<double>(m_yptrack, v_yptrack);
    vectorize_map<double>(m_track_chi2ndf, v_track_chi2ndf);
    vectorize_map<int>(m_track_nhits, v_track_nhits);
    vectorize_map<double>(m_xlocal, v_xlocal); vectorize_map<double>(m_ylocal, v_ylocal);
    vectorize_map<double>(m_zlocal, v_zlocal);
    vectorize_map<int>(m_hit_track_index, v_hit_track_index);
    vectorize_map<int>(m_hit_module, v_hit_module);
}

//
void Tracking::UnitTest()
{
    std::cout<<"Tracking Unit Test."<<std::endl;

    // test combination function
    std::vector<int> layers = {1, 2, 3, 4, 5, 6, 7};
    std::vector<std::vector<int>> res;
    getCombinationList(layers, 6, res);

    Print(layers);
    for(auto &i: res)
        Print(i);
    std::cout<<"total: "<<res.size()<<std::endl;

    // test odometer
    std::vector<int> nhits_by_layer = {4, 3, 2};
    std::vector<int> layer_index = {0, 1, 2};
    std::vector<int> comb;
    std::cout<<"nhits by layer:"<<std::endl;
    Print(nhits_by_layer);
    std::cout<<"combinations: "<<std::endl;
    scanCandidate(nhits_by_layer, layer_index, comb);
}

void Tracking::Print(const std::vector<int> &v) {
    for(auto &i: v)
        std::cout<<std::setfill(' ')<<std::setw(6)<<i;
    std::cout<<std::endl;
}

//
void Tracking::PrintHitStatus()
{
    std::cout<<"hit usage status in current event: "<<std::endl;
    for(auto &i: hit_used) {
        std::cout<<"layer: "<<std::setw(4)<<std::setfill(' ')<<i.first<<std::endl;

        for(bool j: i.second)
            std::cout<<std::setw(2)<<std::setfill(' ')<<j;
        std::cout<<std::endl;
    }
}

//
void Tracking::PrintLayerGroups()
{
    std::cout<<"layer index: "<<std::endl;
    Print(layer_index);
    std::cout<<"minimum hits on track: "<<minimum_hits_on_track<<std::endl;
    std::cout<<"layer combinations: "<<std::endl;
    for(auto &i: group_nlayer) {
        std::cout<<"layer required: "<<i.first<<" : "<<i.second.size()<<std::endl;
        for(auto &j: i.second)
            Print(j);
    }
}

};
