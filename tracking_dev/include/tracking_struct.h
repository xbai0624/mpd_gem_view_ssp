#ifndef TRACKING_STRUCT_H
#define TRACKING_STRUCT_H

#include <cmath>
#include <iostream>

namespace tracking_dev{

struct point_t
{
    double x, y, z;
    double x_charge, y_charge;
    double x_peak, y_peak;
    int x_max_timebin, y_max_timebin;
    int x_size, y_size; // cluster size
    int module_id;
    int layer_id;

    point_t():
        x(0), y(0), z(0), x_charge(0), y_charge(0),
        x_peak(0), y_peak(0), x_max_timebin(-1), y_max_timebin(-1),
        x_size(0), y_size(0), module_id(0), layer_id(0)
    {}

    point_t(double a, double b, double c):
        x(a), y(b), z(c), x_charge(0), y_charge(0),
        x_peak(0), y_peak(0), x_max_timebin(-1), y_max_timebin(-1),
        x_size(0), y_size(0), module_id(0), layer_id(0)
    {}

    point_t(double a, double b, double c, double x_c, double y_c,
            double x_p, double y_p, int x_mt, int y_mt, int x_s, int y_s):
        x(a), y(b), z(c), x_charge(x_c), y_charge(y_c),
        x_peak(x_p), y_peak(y_p), x_max_timebin(x_mt), y_max_timebin(y_mt),
        x_size(x_s), y_size(y_s), module_id(0), layer_id(0)
    {}

    point_t(const point_t &_p):
        x(_p.x), y(_p.y), z(_p.z), x_charge(_p.x_charge),
        y_charge(_p.y_charge), x_peak(_p.x_peak), y_peak(_p.y_peak),
        x_max_timebin(_p.x_max_timebin), y_max_timebin(_p.y_max_timebin),
        x_size(_p.x_size), y_size(_p.y_size), module_id(_p.module_id),
        layer_id(_p.layer_id)
    {}

    point_t unit() const {
        double r = sqrt(x*x + y*y + z*z);

        if(r == 0)
            return point_t(0, 0, 0);

        return point_t(x/r, y/r, z/r);
    }

    point_t &operator=(const point_t &p) {
        if(this == &p)
            return *this;

        x=p.x; y=p.y; z=p.z; x_charge = p.x_charge;
        y_charge = p.y_charge; x_peak = p.x_peak;
        y_peak = p.y_peak; x_size = p.x_size; y_size = p.y_size;
        module_id = p.module_id; layer_id = p.layer_id;

        return *this;
    }

    point_t operator-(const point_t &p) const {
        return point_t(x-p.x, y-p.y, z-p.z);
    }

    point_t operator+(const point_t &p) const {
        return point_t(x+p.x, y+p.y, z+p.z);
    }

    point_t operator*(const double &scale) const {
        return point_t(x*scale, y*scale, z*scale);
    }

    double dot(const point_t &p) const {
        return x*p.x + y*p.y + z*p.z;
    }

    double mod() const {
        return sqrt(x*x + y*y + z*z);
    }
};

std::ostream & operator<<(std::ostream &os, const point_t &p);

struct grid_t {
    double x1, y1, x2, y2;
    grid_t() :x1(0), y1(0), x2(0), y2(0)
    {}

    grid_t(double a, double b, double c, double d)
        : x1(a), y1(b), x2(c), y2(d)
    {}
};

std::ostream & operator<<(std::ostream &os, const grid_t &p);

struct grid_addr_t
{
    int x, y;
    grid_addr_t():x(0), y(0)
    {}

    grid_addr_t(int i, int j) : x(i), y(j)
    {}

    grid_addr_t(const grid_addr_t &t):
        x(t.x), y(t.y)
    {}

    grid_addr_t & operator=(const grid_addr_t &t)
    {
        x = t.x; y = t.y;
        return *this;
    }

    bool operator==(const grid_addr_t &t) const
    {
        if( x == t.x && y == t.y)
            return true;
        return false;
    }

    bool operator<(const grid_addr_t &t) const
    {
        if(x > t.x) return false;
        if(y > t.y) return false;
		return true;
    }

    bool operator>(const grid_addr_t &t) const
    {
        if(y < t.y) return false;
        if(x < t.x) return false;
		return true;
    }
};

std::ostream & operator<<(std::ostream &os, const grid_addr_t &p);

};


// hash grid address structure to make map look up faster
namespace std {
    template<> struct hash<tracking_dev::grid_addr_t>
    {
        std::size_t operator()(const tracking_dev::grid_addr_t &t) const
        {
            return ((t.y & 0xff) | (t.x & 0xff) << 8);
        }
    };
}

#endif
