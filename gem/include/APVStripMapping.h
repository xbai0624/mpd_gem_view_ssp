#ifndef APV_STRIP_MAPPING_H
#define APV_STRIP_MAPPING_H

/*
 *  This file needs to be reorganized in a better way
 *  Improvement will be continuously made
 */

////////////////////////////////////////////////////////////////////////////////
// apv internal channel mapped to detector position
//
// this map includes three stages:
// stage1: apv internal channel number  -> apv chip physical pin index
//         every APV chip has this conversion
//
// stage2: apv chip physical pin index -> on-board (apv hybrid board) panasonic connector pins
//         1) SRS APV hybrid board, INFN MPD APV hybrid board does not need this conversion, 
//            they are connected in the same order
//         2) UVA MPD APV hybrid board needs a conversion, they are not connected one-to-one
//
// stage3: apv hybrid board panasonic connector pins -> sorted strip number on GEM detector
//         1) UVa XY GEM Chambers does not need this conversion, they are connected in the same
//            order
//         2) UVa UV GEM chambers needs a conversion
//         3) PRad GEM chambers needs a conversion
//         4) INFN XY GEM chambers does not need this? (need to confirm): TODO
//
// using this mapping, one get directly map apv internal channel to 
// sorted strip number on GEM chamber
//
// check the APV hybrid board for details

#include <unordered_map>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include "GEMStruct.h"
#include "ConfigObject.h"

namespace apv_strip_mapping {

////////////////////////////////////////////////////////////////////////////////
// UVa XY GEM Detector
// a strip mapping from danning, combines three stages conversion
// this is original and believed to be the right one, Jan 09 2017
// 1) apv internal tree structure mapping:
//    strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);
// 2) apv chip to on-board panasonic connector mapping:
//    strip = ch + 1 + ch%4 - 5 * ( (ch/4) % 2 );
// 3) panasonic connector to detector strip mapping:
//    strip = ch

const int _mapped_strip_uva_xy[128] = {
     1,  33, 65,  97,  9,  41, 73, 105, 17,  49, 
    81, 113, 25,  57, 89, 121,  3,  35, 67,  99, 
    11,  43, 75, 107, 19,  51, 83, 115, 27,  59, 
    91, 123,  5,  37, 69, 101, 13,  45, 77, 109, 
    21,  53, 85, 117, 29,  61, 93, 125,  7,  39, 
    71, 103, 15,  47, 79, 111, 23,  55, 87, 119, 
    31,  63, 95, 127,  0,  32, 64,  96,  8,  40, 
    72, 104, 16,  48, 80, 112, 24,  56, 88, 120, 
     2,  34, 66,  98, 10,  42, 74, 106, 18,  50, 
    82, 114, 26,  58, 90, 122,  4,  36, 68, 100, 
    12,  44, 76, 108, 20,  52, 84, 116, 28,  60, 
    92, 124,  6,  38, 70, 102, 14,  46, 78, 110, 
    22,  54, 86, 118, 30,  62, 94, 126
};

////////////////////////////////////////////////////////////////////////////////
// INFN XY GEM Detector
// a strip mapping, combines three stages conversion
// 1) apv internal tree structure mapping:
//    strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);
// 2) apv chip to on-board panasonic connector mapping:
//    strip = ch
// 3) panasonic connector to detector strip mapping:
//    strip = ch

const int _mapped_strip_infn_xy[128] = {
     0,  32,  64,  96,   8,  40,  72, 104,  16,  48,
    80, 112,  24,  56,  88, 120,   1,  33,  65,  97,
     9,  41,  73, 105,  17,  49,  81, 113,  25,  57,
    89, 121,   2,  34,  66,  98,  10,  42,  74, 106,
    18,  50,  82, 114,  26,  58,  90, 122,   3,  35,
    67,  99,  11,  43,  75, 107,  19,  51,  83, 115,
    27,  59,  91, 123,   4,  36,  68, 100,  12,  44,
    76, 108,  20,  52,  84, 116,  28,  60,  92, 124,
     5,  37,  69, 101,  13,  45,  77, 109,  21,  53,
    85, 117,  29,  61,  93, 125,   6,  38,  70, 102,
    14,  46,  78, 110,  22,  54,  86, 118,  30,  62,
    94, 126,   7,  39,  71, 103,  15,  47,  79, 111,
    23,  55,  87, 119,  31,  63,  95, 127
};

////////////////////////////////////////////////////////////////////////////////
// UVa UV GEM Detector
// a strip mapping, combines three stages conversion
// 1) apv internal tree structure mapping:
//    strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);
// 2) apv chip to on-board panasonic connector mapping:
//    strip = ch + 1 + ch%4 - 5 * ( (ch/4) % 2 );
// 3) panasonic connector to detector strip mapping from Kondo:
//    if(ch %2 == 0)
//        strip = ch/2 + 32;
//    else {
//        if(ch < 64)
//            strip = (63 - ch)/2;
//        else
//            strip = 127 + (65 - ch)/2;
//    }

const int _mapped_strip_uva_uv[128] = {
     31,  15, 127, 111,  27,  11, 123, 107,  23,   7,
    119, 103,  19,   3, 115,  99,  30,  14, 126, 110,
     26,  10, 122, 106,  22,   6, 118, 102,  18,   2,
    114,  98,  29,  13, 125, 109,  25,   9, 121, 105,
     21,   5, 117, 101,  17,   1, 113,  97,  28,  12,
    124, 108,  24,   8, 120, 104,  20,   4, 116, 100,
     16,   0, 112,  96,  32,  48,  64,  80,  36,  52,
     68,  84,  40,  56,  72,  88,  44,  60,  76,  92,
     33,  49,  65,  81,  37,  53,  69,  85,  41,  57,
     73,  89,  45,  61,  77,  93,  34,  50,  66,  82,
     38,  54,  70,  86,  42,  58,  74,  90,  46,  62,
     78,  94,  35,  51,  67,  83,  39,  55,  71,  87,
     43,  59,  75,  91,  47,  63,  79,  95
};

////////////////////////////////////////////////////////////////////////////////
// a hypothetical detector, no conversion, use apv interanl index

const int _mapped_strip_apv_internal[128] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
    20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
    40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
    50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
    70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127
};

////////////////////////////////////////////////////////////////////////////////
// organize strip mappings using unordered_map
const std::unordered_map<std::string, const int* const> mapped_strip_arr = {
    {"UVAXYGEM", _mapped_strip_uva_xy},
    {"UVAUVGEM", _mapped_strip_uva_uv},
    {"INFNXYGEM", _mapped_strip_infn_xy},
    {"INTERNAL", _mapped_strip_apv_internal}
};

////////////////////////////////////////////////////////////////////////////////
// print out mapping
template<typename T>
void print(const typename std::enable_if<std::is_same<T,
        std::unordered_map<std::string, const int* const>>::value, T>::type &m)
{
    for(auto &i: m)
    {
        std::cout<<i.first<<std::endl;
        for(int ii=0; ii<128; ++ii) {
            if(ii%10 == 0 && ii!=0)
                std::cout<<std::endl;
            std::cout<<std::setfill(' ')<<std::setw(6)<<i.second[ii]<<",";
        }
        std::cout<<std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// remove leading and trailing spaces of a string

static std::string strip(const std::string &str)
{
    size_t s = str.find_first_not_of(' ');
    size_t e = str.find_last_not_of(' ');
    size_t len = e - s + 1;

    std::string res = str.substr(s, len);
    return res;
}

////////////////////////////////////////////////////////////////////////////////
// a struct stores all apv information 
// duplicated structure, for compatibility to all previous data generated using
// siyu's code
//
// to be removed

struct APVInfo
{
    int crate_id, layer_id, mpd_id;

    // detector id is an unique number assigned to each GEM during assembly in UVa
    // it is labeled on each detector
    int detector_id;

    // x/y plane (0=x; 1=y; to be changed to string)
    int dimension;
    int adc_ch, i2c_ch, apv_pos, invert;
    std::string discriptor;
    int backplane_id, gem_pos;

    APVInfo(){};
    APVInfo(const std::string &str)
    {
        if(str.find("APV") == std::string::npos)
            return;

        std::istringstream entry(str);
        std::string token;
        std::vector<std::string> tmp;
        while(std::getline(entry, token, ','))
            tmp.push_back(strip(token));
        try{
            crate_id     = std::stoi(tmp[1]);    layer_id    = std::stoi(tmp[2]); 
            mpd_id       = std::stoi(tmp[3]);    detector_id = std::stoi(tmp[4]);
            dimension    = std::stoi(tmp[5]);    adc_ch      = std::stoi(tmp[6]); 
            i2c_ch       = std::stoi(tmp[7]);    apv_pos     = std::stoi(tmp[8]);   
            invert       = std::stoi(tmp[9]);    discriptor  = tmp[10];          
            backplane_id = std::stoi(tmp[11]);   gem_pos     = std::stoi(tmp[12]);
        }
        catch(...){
            std::cout<<__PRETTY_FUNCTION__<<" error enountered..."<<std::endl;
        }
    }
};

std::ostream &operator<<(std::ostream& out, const APVInfo &);

////////////////////////////////////////////////////////////////////////////////
// a struct stores all layer information 

struct LayerInfo
{
    int layer_id, chambers_per_layer;
    std::string readout_type;
    float x_offset, y_offset;
    std::string gem_type;
    int nb_apvs_x, nb_apvs_y;
    float x_pitch, y_pitch;
    int x_flip, y_flip;

    LayerInfo() {};
    LayerInfo(const std::string &str)
    {
        if(str.find("Layer") == std::string::npos)
            return;

        std::istringstream entry(str);
        std::string token;
        std::vector<std::string> tmp;
        while(std::getline(entry, token, ','))
            tmp.push_back(strip(token));
        try{
            layer_id     = std::stoi(tmp[1]);   chambers_per_layer = std::stoi(tmp[2]); 
            readout_type = tmp[3];
            x_offset     = std::stod(tmp[4]);   y_offset           = std::stod(tmp[5]); 
            gem_type     = tmp[6];
            nb_apvs_x    = std::stoi(tmp[7]);   nb_apvs_y          = std::stoi(tmp[8]);          
            x_pitch      = std::stod(tmp[9]);   y_pitch            = std::stod(tmp[10]);
            x_flip       = std::stoi(tmp[11]);  y_flip             = std::stoi(tmp[12]);
        }
        catch(...){
            std::cout<<__PRETTY_FUNCTION__<<" error enountered..."<<std::endl;
        }
    }
};

std::ostream &operator<<(std::ostream& out, const LayerInfo &);


////////////////////////////////////////////////////////////////////////////////
// a gem mapping class
// this class is redundant, to be removed in the future

class Mapping 
{
public:
    static Mapping* Instance() {
        if(!instance)
            instance = new Mapping();
        return instance;
    }

    Mapping(Mapping const &) = delete;
    void operator=(Mapping const &) = delete;

    ~Mapping(){}

    void LoadMap(const char* path);
    void Print();

    // members
    void ExtractMPDAddress();
    void ExtractAPVAddress();
    void ExtractDetectorID();
    void ExtractLayerID();

    // getters
    int GetPlaneID(const GEMChannelAddress &addr);
    int GetProdID(const GEMChannelAddress &addr);
    int GetModuleID(const GEMChannelAddress &addr);
    int GetAxis(const GEMChannelAddress &addr);
    int GetStrip(const std::string &detector_type, const GEMChannelAddress &addr);
    int GetTotalNumberOfDetectors();
    int GetTotalNumberOfLayers();
    int GetTotalNumberOfAPVs() const;

    int GetTotalMPDs();
    const std::vector<MPDAddress> & GetMPDAddressVec() const;
    const std::vector<APVAddress> & GetAPVAddressVec() const;
    const std::vector<int> & GetLayerIDVec() const;
    const std::map<int, LayerInfo> & GetLayerMap() const;

private:
    static Mapping* instance;
    Mapping();

    std::unordered_map<APVAddress, APVInfo> apvs;
    std::vector<MPDAddress> vMPDAddr;
    std::vector<APVAddress> vAPVAddr;
    std::vector<int> vDetID;
    std::vector<int> vLayerID;

    std::map<int, LayerInfo> layers;

    bool map_loadded = false;

    ConfigObject txt_parser;
};

};
#endif
