#include "InfoCenter.h"

#include <iostream>

InfoCenter* InfoCenter::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////
// dtor

InfoCenter::~InfoCenter()
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// parse run number from input name
// currently support:
//        xxx[_.-(a-z)(A-Z)]334.evio.0
//         xxx[_.-(a-z)(A-Z)]334.dat.0

int InfoCenter::ParseRunNumber(const std::string &input)
{
    int res = -1;
    size_t pos_start = 0;
    if( input.find(".evio") != std::string::npos) {
        pos_start = input.find(".evio");
    }
    else if(input.find(".dat") != std::string::npos) {
        pos_start = input.find(".dat");
    }
    else {
        std::cout<<__func__<<" Warning: only evio/dat files are accepted: "<<input
                 <<std::endl;
        return -1;
    }

    int not_digit = static_cast<int>(pos_start) - 1;
    for(; not_digit>=0; --not_digit)
    {
        int c = static_cast<int>(input[not_digit]);
        if(c < 48 || c > 57)
            break;
    }

    int length = pos_start - not_digit - 1;
    if(length <= 0) 
        return -1;

    std::string _run = input.substr(not_digit+1, length);
    res = std::stoi(_run);

    return res;
}
