#ifndef TRACKING_H
#define TRACKING_H

#include <unordered_map>
#include <vector>
#include <iomanip>
#include "tracking_struct.h"
#include "Cuts.h"

namespace tracking_dev {

    class TrackingUtility;
    class AbstractDetector;

#define LARGE_VALUE 999999999.

class Tracking
{
public:
    Tracking();
    ~Tracking();

    void AddDetector(int index, AbstractDetector*);
    void CompleteSetup();
    void FindTracks();
    void ClearPreviousEvent();

    // unit test
    void UnitTest();
    void Print(const std::vector<int> &v);
    void PrintHitStatus();
    void PrintLayerGroups();

    // getters for best track
    bool GetBestTrack(double &xt, double &yt, double &xp, double &yp, double &chi);
    int GetNHitsonBestTrack(){return nhits_on_best_track;}
    const std::vector<int> &GetBestTrackLayerIndex(){return best_track_layer_index;}
    const std::vector<int> &GetBestTrackHitIndex(){return best_track_hit_index;}

    // getters for all good tracks that pass chi2 cut
    int GetNGoodTrackCandidates(){return n_good_track_candidates;}
    int GetBestTrackIndex(){return best_track_index;}
    const std::vector<double> & GetAllXtrack() const {return v_xtrack;}
    const std::vector<double> & GetAllYtrack() const {return v_ytrack;}
    const std::vector<double> & GetAllXptrack() const {return v_xptrack;}
    const std::vector<double> & GetAllYptrack() const {return v_yptrack;}
    const std::vector<double> & GetAllChi2ndf() const {return v_track_chi2ndf;}
    const std::vector<int> & GetAllTrackNhits() const {return v_track_nhits;}
    int GetTotalNgoodHits() {return n_total_good_hits;}
    const std::vector<double> & GetAllXlocal() const {return v_xlocal;}
    const std::vector<double> & GetAllYlocal() const {return v_ylocal;}
    const std::vector<double> & GetAllZlocal() const {return v_zlocal;}
    const std::vector<int> & GetAllHitTrackIndex() const {return v_hit_track_index;}
    const std::vector<int> & GetAllHitModule() const {return v_hit_module;}


    TrackingUtility* GetTrackingUtility() {return tracking_utility;}

private:
    void initHitStatus();
    void initLayerGroups();
    void loopAllLayerGroups();

    void nextLayerGroup(const std::vector<int> &group);
    void scanCandidate(const std::vector<int> &nhit_by_layer,
            const std::vector<int> &layer_index,
            std::vector<int> &hit_comb);

    void nextLayerGroup_gridway(const std::vector<int> &group);
    void scanCandidate_gridway(const int &p_start, const int &p_start_index,
            const int &p_end, const int &p_end_index,
            const std::vector<int> &middle_layers);
    void scanCandidate_gridway(const std::unordered_map<int, std::vector<int>> &vhitid_by_layer,
            std::vector<int> layer_combo, std::vector<int> hit_combo,
            const std::vector<int> &middle_layer, int remaining_layer);
    void getMiddleLayerGridHitIndex(const int &start, const int &start_index,
            const int &end, const int &end_index,
            const std::vector<int> &middle_layers,
            std::unordered_map<int, std::vector<int>> &hit_index_by_layer);

    // track fitting
    void nextTrackCandidate(const std::vector<std::pair<int, int>> &combination);
    void nextTrackCandidate(const std::vector<point_t> &combination);
    void nextTrackCandidate(const std::vector<int> &layer_index, const std::vector<int> &hit_index);
    bool found_tracks_with_nlayer(int nlayer);

private:
    void getCombinationList(const std::vector<int> &layers, const int &m,
            std::vector<std::vector<int>>& res);

private:
    TrackingUtility *tracking_utility;
    Cuts *tracking_cuts;

    std::unordered_map<int, AbstractDetector*> detector; // layer_id <-> detector
    std::vector<int> layer_index; // vector of layer_id

    std::unordered_map<int, std::vector<bool>> hit_used; // layer_index <-> detector hit status

    int minimum_hits_on_track = 3;
    double chi2_cut = 10;

    // optics cut
    double k_min_yz = -9999, k_max_yz = 9999;
    double k_min_xz = -9999, k_max_xz = 9999;

    // all possible groups
    std::unordered_map<int, std::vector<std::vector<int>>> group_nlayer;

    // cache current working combination
    std::vector<int> current_layer_comb; // optional, as (xtrack, ytrack), (xptrack, yptrack) is enough
    std::vector<int> current_hit_comb;   // optional, as (xtrack, ytrack), (xptrack, yptrack) is enough

    // tracking result - best track
    int best_track_index;
    int nhits_on_best_track;
    std::vector<int> best_track_layer_index; // optional, as (xtrack, ytrack), (xptrack, yptrack) is enough
    std::vector<int> best_track_hit_index;   // optional, as (xtrack, ytrack), (xptrack, yptrack) is enough
    double best_track_chi2ndf = LARGE_VALUE;
    double best_xtrack = LARGE_VALUE, best_ytrack = LARGE_VALUE;
    double best_xptrack = LARGE_VALUE, best_yptrack = LARGE_VALUE;
    //
    std::unordered_map<int, double> best_track_chi2ndf_by_nlayer;

    // tracking result - all good tracks that pass chi2 cut
    // all possible track candidates, this is not exclusive.
    // for example, if hit_1 is used by track_candidate_1, it can also be used by track_candidate_2
    // this number estimate all possible combinations, b/c each combination have the same weight (we don't
    // know how to assign weight to a track).
    int n_good_track_candidates = 0;
    std::vector<double> v_xtrack, v_ytrack, v_xptrack, v_yptrack, v_track_chi2ndf;
    std::vector<int> v_track_nhits;

    int n_total_good_hits;
    std::vector<double> v_xlocal, v_ylocal, v_zlocal;
    std::vector<int> v_hit_track_index;
    std::vector<int> v_hit_module;

    // debug
    //std::vector<point_t> best_hits_on_track;
};

};

#endif
