#include "MPDDataStruct.h"
#include <iomanip>

////////////////////////////////////////////////////////////////
// initialize all static memebers


////////////////////////////////////////////////////////////////
// overload MPDAddress

std::ostream& operator<<(std::ostream& out, const MPDAddress &addr)
{
    out<<std::setfill(' ')<<std::setw(8)<<addr.crate_id
        <<std::setfill(' ')<<std::setw(8)<<addr.mpd_id;
    return out;
}

////////////////////////////////////////////////////////////////
// overload APVAddress

std::ostream &operator<<(std::ostream &out, const APVAddress &ad)
{
    out << std::setfill(' ')<<std::setw(8) << ad.crate_id 
        << std::setfill(' ')<<std::setw(8) << ad.mpd_id 
        << std::setfill(' ')<<std::setw(8) << ad.adc_ch;
    return out;
}

