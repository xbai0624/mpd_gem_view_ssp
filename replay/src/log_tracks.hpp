#ifndef LOG_TRACKS_HPP
#define LOG_TRACKS_HPP

#include <fstream>
#include <iostream>
#include <iomanip>
#include "tracking_struct.h"

namespace log_tracks {

inline void open_tracks_text_file()
{
    std::fstream f("log_tracks.txt", std::ios::out | std::ios::trunc);
    if(!f.is_open()) {
        std::cout<<"ERROR: cannot open text file tracks.txt to write tracks."<<std::endl;
        exit(1);
    }
    f<<std::setw(10)<<std::setfill(' ')<<"module_id"
        <<std::setw(12)<<std::setfill(' ')<<"x"
        <<std::setw(12)<<std::setfill(' ')<<"y"
        <<std::setw(12)<<std::setfill(' ')<<"z"
        <<std::endl;
}

inline void append_track(const std::vector<tracking_dev::point_t> & track)
{
    std::fstream f("log_tracks.txt", std::ios::out | std::ios::app);
    if(!f.is_open()){
        std::cout<<"ERROR: cannot open text file tracks.txt to write tracks."<<std::endl;
        exit(1);
    }

    for(const auto &hit: track) {
        double x = hit.x, y = hit.y, z = hit.z;
        int module_id = hit.module_id;

        f<<std::setw(10)<<std::setfill(' ')<<module_id
            <<std::setw(12)<<std::setfill(' ')<<x
            <<std::setw(12)<<std::setfill(' ')<<y
            <<std::setw(12)<<std::setfill(' ')<<z
            <<"        ";
    }
    f<<std::endl;
}

};

#endif
