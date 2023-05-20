#ifndef ABSTRACT_DETECTOR_H
#define ABSTRACT_DETECTOR_H

#include "tracking_struct.h"
#include <vector>
#include <unordered_map>

namespace tracking_dev {

class AbstractDetector
{
public:
    AbstractDetector();
    ~AbstractDetector();

    void SetOrigin(const point_t &p);
    void SetXAxis(const point_t &p);
    void SetYAxis(const point_t &p);
    void SetZAxis(const point_t &p);
    void SetDimension(const point_t &p);

    void AddLocalHit(const point_t &p);
    void AddGlobalHit(const point_t &p);
    void AddHit(const point_t &p) { AddGlobalHit(p); }
    void AddHit(const double &x, const double &y);
    void AddFittedHits(const point_t &p) { addNonIndexHit(p, fitted_hits); }
    void AddRealHits(const point_t &p) { addNonIndexHit(p, real_hits); }
    void AddBackgroundHits(const point_t &p) { addNonIndexHit(p, background_hits); }

    // getters
    const point_t &GetOrigin() const;
    double GetZPosition() const;
    const point_t &GetXAxis() const;
    const point_t &GetYAxis() const;
    const point_t &GetZAxis() const;
    const point_t &GetDimension() const;
    const std::vector<point_t> &GetLocalHits() const;
    const std::vector<point_t> &GetGlobalHits() const;
    const std::vector<point_t> &GetHits() const {return global_hits;}
    const point_t &Get2DHit(int i) const {return global_hits[i];}
    unsigned int Get2DHitCounts() const {return global_hits.size();}
    const std::unordered_map<grid_addr_t, grid_t> &GetGrids() const {return grids;}
    const std::unordered_map<grid_addr_t, bool> &GetGridChosen() const {return grid_chosen;}
    const std::unordered_map<grid_addr_t, std::vector<int>> &GetGridVHits() const {return vhits_by_grid;}
    std::vector<grid_addr_t> GetPointHomeGrids(const point_t &p);
    int GetGridNeighborStatus(const point_t &p, const grid_addr_t &a);
    const std::vector<point_t> &GetFittedHits() const {return fitted_hits;}
    const std::vector<point_t> &GetRealHits() const {return real_hits;}
    const std::vector<point_t> &GetBackgroundHits() const {return background_hits;}

    // members
    void Reset();
    void SetupGrids();
    void ShowGridHitStat();

    // setters
    void SetGridWidth(double xw, double yw){ grid_xwidth = xw; grid_ywidth = yw;}
    void SetGridShift(double shift) {grid_shift = shift;}

public:
    void addNonIndexHit(const point_t &p, std::vector<point_t> &hits);
    void addIndexHit(const point_t &p);

private:
    point_t origin;
    point_t z_axis;
    point_t x_axis;
    point_t y_axis;
    point_t dimension; // total length, not half length

    // local hits is only used for detector raw signal check
    std::vector<point_t> local_hits;
    // for 2D hits, all hits, in global coordinates
    std::vector<point_t> global_hits;

    // test - in global coordinates
    std::vector<point_t> fitted_hits;
    std::vector<point_t> real_hits;
    std::vector<point_t> background_hits;

    // grid
    double grid_xwidth = 17.2, grid_ywidth = 17.2; // units in mm
    double grid_shift = 0.4;
    //double grid_xwidth = 102.4, grid_ywidth = 102.4; // units in mm
    //double grid_shift = 0.;
    double neighbor_grid_marginx = 0.3; // default is 1/4 grid width
    double neighbor_grid_marginy = 0.3;
 
    std::unordered_map<grid_addr_t, grid_t> grids;
    std::unordered_map<grid_addr_t, bool> grid_chosen;
    std::unordered_map<grid_addr_t, std::vector<int>> vhits_by_grid;
};

};

#endif
