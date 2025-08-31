#include "AbstractDetector.h"
#include <cmath>

namespace tracking_dev {

AbstractDetector::AbstractDetector() : origin(point_t(0, 0, 0)),
    z_axis(point_t(0, 0, 1.)), x_axis(point_t(1, 0, 0)), 
    y_axis(point_t(0, 1., 0)), dimension(point_t(1, 1, 1))
{
    local_hits.clear(); global_hits.clear();
    real_hits.clear(); fitted_hits.clear(); background_hits.clear();
}

AbstractDetector::~AbstractDetector()
{
}

void AbstractDetector::SetOrigin(const point_t &p)
{
    origin = p;
}

void AbstractDetector::SetXAxis(const point_t &p)
{
    x_axis = p;
}

void AbstractDetector::SetYAxis(const point_t &p)
{
    y_axis = p;
}

void AbstractDetector::SetZAxis(const point_t &p)
{
    z_axis = p;
}

void AbstractDetector::SetDimension(const point_t &p)
{
    dimension = p;

    // xb_debug
    SetupGrids();
}

void AbstractDetector::AddLocalHit(const point_t &p)
{
    addNonIndexHit(p, local_hits);
}

void AbstractDetector::AddGlobalHit(const point_t &p)
{
    addIndexHit(p);
}

const point_t &AbstractDetector::GetOrigin() const
{
    return origin;
}

double AbstractDetector::GetZPosition() const
{
    return origin.z;
}

const point_t &AbstractDetector::GetXAxis() const
{
    return x_axis;
}

const point_t &AbstractDetector::GetYAxis() const
{
    return y_axis;
}

const point_t &AbstractDetector::GetZAxis() const
{
    return z_axis;
}

const point_t &AbstractDetector::GetDimension() const
{
    return dimension;
}

const std::vector<point_t> &AbstractDetector::GetLocalHits() const
{
    return local_hits;
}

const std::vector<point_t> &AbstractDetector::GetGlobalHits() const
{
    return global_hits;
}

void AbstractDetector::Reset()
{
    local_hits.clear(); global_hits.clear();
    real_hits.clear(); fitted_hits.clear(); background_hits.clear();

    // reset grid counters
    for(auto &i: grid_chosen)
        i.second = false;
    for(auto &i: vhits_by_grid)
        i.second.clear();
}

void AbstractDetector::AddHit(const double &x, const double &y)
{
    point_t p(x, y, origin.z);
    addIndexHit(p);
}

// hit position doesn't need to be kept record of (mainly for showing purpose)
void AbstractDetector::addNonIndexHit(const point_t &p, std::vector<point_t> &hits)
{
    hits.push_back(p);
}

// hit position needs to be kept record of (for tracking grid search)
inline void AbstractDetector::addIndexHit(const point_t &p)
{
    global_hits.push_back(p);

    size_t index = global_hits.size() - 1;

    double x_low = -dimension.x/2. - grid_shift;
    double y_low = -dimension.y/2. - grid_shift;

    int i = (p.x - x_low)/grid_xwidth;
    int j = (p.y - y_low)/grid_ywidth;

    grid_addr_t addr(i, j);
    vhits_by_grid[addr].push_back(index);
}

void AbstractDetector::SetupGrids()
{
    double width = dimension.x, height = dimension.y;
    int nbinsx = std::ceil((width + grid_shift)/grid_xwidth);
    int nbinsy = std::ceil((height + grid_shift)/grid_ywidth);

    for(int i=0; i<nbinsx; i++)
    {
        double x_low = i*grid_xwidth - grid_shift - width/2.;
        double x_high = (i+1)*grid_xwidth - grid_shift - width/2.;

        for(int j=0; j<nbinsy; j++)
        {
            double y_low = j*grid_ywidth - grid_shift - height/2.;
            double y_high = (j+1)*grid_ywidth - grid_shift - height/2.;

            grid_addr_t addr(i, j);
            grid_t g(x_low, y_low, x_high, y_high);

            grids[addr] = g;
            grid_chosen[addr] = false;
        }
    }

    // if point to grid edge distance is smaller than neighbor_grid_margin
    // then include this neighbor grid
    neighbor_grid_marginx = grid_xwidth / 3.;
    neighbor_grid_marginy = grid_ywidth / 3.;
}

std::vector<grid_addr_t> AbstractDetector::GetPointHomeGrids(const point_t &p)
{
    double x_low = -dimension.x/2. - grid_shift;
    double y_low = -dimension.y/2. - grid_shift;

    int i = (p.x - x_low) / grid_xwidth;
    int j = (p.y - y_low) / grid_ywidth;

    grid_addr_t addr(i, j);

    std::vector<grid_addr_t> res;

    if(grids.find(addr) == grids.end())
        return res;

    res.push_back(addr);

    int status = GetGridNeighborStatus(p, addr);

    auto add_grid = [&](int a, int b)
    {
        grid_addr_t tmp(a, b);
        if(grids.find(tmp) != grids.end())
            res.push_back(tmp);
    };

    switch(status) {
        case 0:
            break;
        case 1:
            add_grid(i-1, j);
            break;
        case 2:
            { add_grid(i-1, j); add_grid(i, j+1); add_grid(i-1, j+1); }
            break;
        case 3:
            add_grid(i, j+1);
            break;
        case 4:
            { add_grid(i+1, j); add_grid(i+1, j+1); add_grid(i, j+1); }
            break;
        case 5:
            add_grid(i+1, j);
            break;
        case 6:
            { add_grid(i+1, j); add_grid(i+1, j-1); add_grid(i, j-1); }
            break;
        case 7:
            add_grid(i, j-1);
            break;
        case 8:
            { add_grid(i-1, j); add_grid(i-1, j-1); add_grid(i, j-1); }
            break;
        default:
            break;
    };

    return res;
}

// grid neighbor status
// 2---3---4
// -       -
// 1   0   5
// -       -
// 8---7---6
int AbstractDetector::GetGridNeighborStatus(const point_t &p, const grid_addr_t &addr)
{
    double x_left = p.x - grids.at(addr).x1;
    double x_right = grids.at(addr).x2 - p.x;
    double y_bottom = p.y - grids.at(addr).y1;
    double y_top = grids.at(addr).y2 - p.y;

    if(x_left < neighbor_grid_marginx){
        if(y_top < neighbor_grid_marginy)
            return 2;
        else if(y_bottom < neighbor_grid_marginy)
            return 8;
        return 1;
    }

    if(x_right < neighbor_grid_marginx) {
        if(y_top < neighbor_grid_marginy)
            return 4;
        else if(y_bottom < neighbor_grid_marginy)
            return 6;
        return 5;
    }

    if(y_top < neighbor_grid_marginy)
        return 3;

    if(y_bottom < neighbor_grid_marginy)
        return 7;

    return 0;
}

void AbstractDetector::ShowGridHitStat()
{
    for(auto &i: vhits_by_grid)
    {
        std::cout<<i.first<<": "<<i.second.size()<<std::endl;
    }
}

};
