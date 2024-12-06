#ifndef MOLLER_RO_STRIP_H
#define MOLLER_RO_STRIP_H

/******************************************************************************
 *
 *  The Moller RO strip information is read directly from Moller Desing Gerber
 *  Files, email Nilanga and UVA group memebers if you need these design files
 *
 * ****************************************************************************/


#include <unordered_map>
#include <fstream>

// For Moller RO board, both U(top) side and V(bottom) side has 604 strips in total
#define MOLLER_RO_SIZE 604

struct moller_strip_t {
    int strip_no;
    double x_start, y_start, x_end, y_end;

    moller_strip_t(): strip_no(0), x_start(0),
                      y_start(0), x_end(0), y_end(0)
    {
    }

    moller_strip_t(int n, double x0, double y0, double x1, double y1)
        : strip_no(n), x_start(x0), y_start(y0), x_end(x1), y_end(y1)
    {
    }
};

inline std::unordered_map<int, moller_strip_t> moller_top_strips; // u strips
inline std::unordered_map<int, moller_strip_t> moller_bot_strips; // v strips

inline bool moller_strips_loaded = false;

inline void load_moller_strip_design_files()
{
    if(moller_strips_loaded) return;

    std::fstream f1("config/moller_strip_design/Moller_GEM_RO_Top_Strip_Coord.txt", std::fstream::in);
    if(!f1.is_open()) {
	    std::cout<<"Cannot open Moller chamber strip design files:"<<std::endl;
	    std::cout<<"      config/moller_strip_design/Moller_GEM_RO_Top_Strip_Coord.txt"<<std::endl;
	    exit(0);
    }
    int nb;
    double x_start, y_start, x_end, y_end;
    while(f1 >> nb >> x_start >> y_start >> x_end >> y_end)
    {
        moller_strip_t s(nb, x_start, y_start, x_end, y_end);
        moller_top_strips[nb] = s;
    }

    std::fstream f2("config/moller_strip_design/Moller_GEM_RO_Bot_Strip_Coord.txt", std::fstream::in);
    while(f2 >> nb >> x_start >> y_start >> x_end >> y_end)
    {
        moller_strip_t s(nb, x_start, y_start, x_end, y_end);
        moller_bot_strips[nb] = s;
    }

    moller_strips_loaded = true;
}

inline double cross_product(double x1, double y1, double x2, double y2)
{
    return x1 * y2 - y1 * x2;
}

///////////////////////////////////////////////////////////////////////////////
///
/// It is better to start from bottom strips, and loop through top strips, check
/// if a top strip has intersecting point with it. Not the other way around,
/// this is due to precision issue in reading Gerber file
///                               -- use this way to ensure no missing strips
///
///////////////////////////////////////////////////////////////////////////////

inline bool has_intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    double r1 = x2 - x1, r2 = y2 - y1;
    double s1 = x4 - x3, s2 = y4 - y3;

    double d1 = x3 - x1, d2 = y3 - y1;

    double r_cross_s = cross_product(r1, r2, s1, s2);
    double d_cross_r = cross_product(d1, d2, r1, r2);
    double d_cross_s = cross_product(d1, d2, s1, s2);

    // check if lines are parallel
    if(fabs(r_cross_s) < 1e-9) {
        // check if collinaear
        if(fabs(d_cross_r) < 1e-9) {
            // check for overlapping using projections
            double t0 = (d1 * r1 + d2 * r2) / (r1 * r1 + r2 * r2);
            double t1 = ((x3 - x1 + s1) * r1 + (y3 - y1 + s2) * r2) / (r1 * r1 + r2 * r2);
            return fmax(0.0, fmin(t0, t1)) <= fmin(1.0, fmax(t0, t1));
        }
        return false;
    }

    // compute parameters t and u
    double t = d_cross_s / r_cross_s;
    double u = d_cross_r / r_cross_s;

    // check if intersection point is within both segments
    return t >= 0 && t <= 1 && u >= 0 && u <= 1;
    return false;
}

inline bool has_intersect_top_bot(int i, int j)
{
    if( i<0 || i > 604 || j < 0 || j > 604) {
        //std::cout<<"Error: top: "<<i<<" bottom: "<<j<<" not exist."<<std::endl;
	return false;
    }
    const auto &s1 = moller_top_strips.at(i);
    const auto &s2 = moller_bot_strips.at(j);

    double x1 = s1.x_start, y1 = s1.y_start;
    double x2 = s1.x_end, y2 = s1.y_end;
    double x3 = s2.x_start, y3 = s2.y_start;
    double x4 = s2.x_end, y4 = s2.y_end;

    return has_intersect(x1, y1, x2, y2, x3, y3, x4, y4);
}

inline bool has_intersect_bot_top(int i, int j)
{
    return has_intersect_top_bot(j, i);
}

#endif
