#include "tracking_struct.h"
#include <iomanip>

namespace tracking_dev {

std::ostream & operator<<(std::ostream &os, const point_t &p)
{
    os<<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.x
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.y
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.z
        <<std::endl;
    return os;
}

std::ostream & operator<<(std::ostream &os, const grid_t &p)
{
    os<<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.x1
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.y1
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.x2
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.y2
        <<std::endl;
    return os;
}

std::ostream & operator<<(std::ostream &os, const grid_addr_t &p)
{
    os<<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.x
        <<std::setfill(' ')<<std::setw(12)<<std::setprecision(4)<<p.y;
    return os;
}

};
