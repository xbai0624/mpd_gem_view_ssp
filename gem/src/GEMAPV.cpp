//============================================================================//
// GEM APV class                                                              //
// APV is the basic unit for GEM DAQ system, it will be connected to GEM Plane//
//                                                                            //
// Chao Peng      10/07/2016                                                  //
// Xinzhan Bai    12/01/2020                                                  //
//============================================================================//

#include <iostream>
#include <iomanip>
#include <cmath>
#include "MPDDataStruct.h"
#include "GEMSystem.h"
#include "GEMMPD.h"
#include "GEMPlane.h"
#include "GEMAPV.h"
#include "APVStripMapping.h"
#include "TF1.h"
#include "TH1.h"
#include "hardcode.h"

////////////////////////////////////////////////////////////////////////////////
// macro to get the data index
#define DATA_INDEX(ch, ts) (ts_begin + ch + ts*MPD_APV_TS_LEN)

////////////////////////////////////////////////////////////////////////////////
// use vector or TH1I for storing temporal data for pedestal generation
// vector is faster than TH1I
#define USE_VEC 1
//#include <TFile.h>

//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

////////////////////////////////////////////////////////////////////////////////
// constructor

GEMAPV::GEMAPV(const int &o,
        const int &det_pos,
        [[maybe_unused]]const std::string &s, // reserved parameter for apv discription
        const uint32_t &t,
        const float &cth,
        const float &zth,
        const float &ctth,
        const bool &fpga_onlinezerosup)
    : orient(o), detector_position(det_pos),
    common_thres(cth), zerosup_thres(zth), crosstalk_thres(ctth),
    online_zero_suppression(fpga_onlinezerosup)
{
    // initialize
    initialize();

    raw_data = nullptr;
    SetTimeSample(t);

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        offset_hist[i] = nullptr;
        noise_hist[i] = nullptr;
    }

    ClearData();
}

////////////////////////////////////////////////////////////////////////////////
// only used in constructors
// initialize the members that should not be copied

void GEMAPV::initialize()
{
    // these can only be assigned by a MPD (SetMPD)
    mpd = nullptr;
    crate_id = -1;
    mpd_id = -1;
    adc_ch = -1;

    // these can only be assigned by a Plane (SetDetectorPlane)
    plane = nullptr;
    plane_index = -1;
}

////////////////////////////////////////////////////////////////////////////////
// The copy and move constructor/assignment operator won't copy or replace the
// current connection between APV and Plane
// copy constructor

GEMAPV::GEMAPV(const GEMAPV &that)
    : orient(that.orient), detector_position(that.detector_position), 
    time_samples(that.time_samples), 
    common_thres(that.common_thres), zerosup_thres(that.zerosup_thres),
    crosstalk_thres(that.crosstalk_thres), 
    online_zero_suppression(that.online_zero_suppression)
{
    initialize();

    // raw data related
    buffer_size = that.buffer_size;
    ts_begin = that.ts_begin;
    // dangerous part, may fail due to lack of memory
    raw_data = new float[buffer_size];
    // copy values
    for(uint32_t i = 0; i < buffer_size; ++i)
    {
        raw_data[i] = that.raw_data[i];
    }

    // copy other arrays
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        pedestal[i] = that.pedestal[i];
        strip_map[i] = that.strip_map[i];
        hit_pos[i] = that.hit_pos[i];

        // dangerous part, may fail due to lack of memory
        if(that.offset_hist[i] != nullptr) {
            offset_hist[i] = new TH1I(*that.offset_hist[i]);
        } else {
            offset_hist[i] = nullptr;
        }

        if(that.noise_hist[i] != nullptr) {
            noise_hist[i] = new TH1I(*that.noise_hist[i]);
        } else {
            noise_hist[i] = nullptr;
        }
    }

    // copy offline common mode
    offline_common_mode.clear();
    for(auto &i: that.offline_common_mode)
        offline_common_mode.push_back(i);
    // copy online common mode
    online_common_mode.clear();
    for(auto &i: that.online_common_mode)
        online_common_mode.push_back(i);
}

////////////////////////////////////////////////////////////////////////////////
// move constructor

GEMAPV::GEMAPV(GEMAPV &&that)
    : orient(that.orient), detector_position(that.detector_position), 
    time_samples(that.time_samples), 
    common_thres(that.common_thres), zerosup_thres(that.zerosup_thres),
    crosstalk_thres(that.crosstalk_thres),
    online_zero_suppression(that.online_zero_suppression)
{
    initialize();

    // raw_data related
    buffer_size = that.buffer_size;
    ts_begin = that.ts_begin;
    raw_data = that.raw_data;
    // null the pointer of that
    that.buffer_size = 0;
    that.raw_data = nullptr;

    // other arrays
    // static array, so no need to move, just copy elements
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        pedestal[i] = that.pedestal[i];
        strip_map[i] = that.strip_map[i];
        hit_pos[i] = that.hit_pos[i];

        // these need to be moved
        offset_hist[i] = that.offset_hist[i];
        noise_hist[i] = that.noise_hist[i];
        that.offset_hist[i] = nullptr;
        that.noise_hist[i] = nullptr;
    }

    // common mode
    offline_common_mode.clear();
    for(auto &i: that.offline_common_mode)
        offline_common_mode.push_back(i);
    online_common_mode.clear();
    for(auto &i: that.online_common_mode)
        online_common_mode.push_back(i);
}

////////////////////////////////////////////////////////////////////////////////
// destructor

GEMAPV::~GEMAPV()
{
    UnsetMPD();
    UnsetDetectorPlane();
    ReleasePedHist();

    delete[] raw_data;
}

////////////////////////////////////////////////////////////////////////////////
// copy assignment operator

GEMAPV &GEMAPV::operator= (const GEMAPV &rhs)
{
    if(this == &rhs)
        return *this;

    GEMAPV apv(rhs); // use copy constructor
    *this = std::move(apv); // use move assignment operator
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// move assignment operator

GEMAPV &GEMAPV::operator= (GEMAPV &&rhs)
{
    if(this == &rhs)
        return *this;

    // release memory
    ReleasePedHist();
    delete[] raw_data;

    // members
    time_samples = rhs.time_samples;
    orient = rhs.orient;
    common_thres = rhs.common_thres;
    zerosup_thres = rhs.zerosup_thres;
    crosstalk_thres = rhs.crosstalk_thres;
    online_zero_suppression = rhs.online_zero_suppression;

    // raw_data related
    buffer_size = rhs.buffer_size;
    ts_begin = rhs.ts_begin;
    raw_data = rhs.raw_data;
    // null the pointer of that
    rhs.buffer_size = 0;
    rhs.raw_data = nullptr;

    // other arrays
    // static array, so no need to move, just copy elements
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        pedestal[i] = rhs.pedestal[i];
        strip_map[i] = rhs.strip_map[i];
        hit_pos[i] = rhs.hit_pos[i];

        // these need to be moved
        offset_hist[i] = rhs.offset_hist[i];
        noise_hist[i] = rhs.noise_hist[i];
        rhs.offset_hist[i] = nullptr;
        rhs.noise_hist[i] = nullptr;
    }

    // common mode
    offline_common_mode.clear();
    for(auto &i: rhs.offline_common_mode)
        offline_common_mode.push_back(i);
    online_common_mode.clear();
    for(auto &i: rhs.online_common_mode)
        online_common_mode.push_back(i);

    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//


////////////////////////////////////////////////////////////////////////////////
// set apv address (should never be used, b/c address is read from mapping file)

void GEMAPV::SetAddress(const APVAddress &apv_addr)
{
    crate_id = apv_addr.crate_id;
    mpd_id = apv_addr.mpd_id;
    adc_ch = apv_addr.adc_ch;
}


////////////////////////////////////////////////////////////////////////////////
// connect the apv to GEM MPD

void GEMAPV::SetMPD(GEMMPD *m, int slot, bool force_set)
{
    if(m == mpd && slot == adc_ch)
        return;

    if(!force_set)
        UnsetMPD();

    if(m) {
        mpd = m;
        crate_id = mpd->GetCrateID();
        mpd_id = mpd->GetID();
        adc_ch = slot;
    }
}

////////////////////////////////////////////////////////////////////////////////
// disconnect the mpd, reset mpd id and adc ch

void GEMAPV::UnsetMPD(bool force_unset)
{
    if(!mpd)
        return;

    if(!force_unset)
        mpd->DisconnectAPV(adc_ch, true);

    mpd = nullptr;
    crate_id = -1;
    mpd_id = -1;
    adc_ch = -1;
}

////////////////////////////////////////////////////////////////////////////////
// connect the apv to GEM Plane

void GEMAPV::SetDetectorPlane(GEMPlane *p, int slot, bool force_set)
{
    if(p == plane && slot == plane_index)
        return;

    if(!force_set)
        UnsetDetectorPlane();

    if(p) {
        plane = p;
        plane_index = slot;
        // strip map is related to plane that connected, thus build the map
        buildStripMap();
    }
}

////////////////////////////////////////////////////////////////////////////////
// disconnect the plane, reset plane index

void GEMAPV::UnsetDetectorPlane(bool force_unset)
{
    if(!plane)
        return;

    if(!force_unset)
        plane->DisconnectAPV(plane_index, true);

    plane = nullptr;
    plane_index = -1;
}

////////////////////////////////////////////////////////////////////////////////
// create ped histograms

void GEMAPV::CreatePedHist()
{
    // we switched from using TH1I to using vector, no need to create histos
    // anymore. This function was kept for future use
#ifdef USE_VEC
    return;
#else
    // obsolete
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if(offset_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_OFFSET_"
                + std::to_string(crate_id) + "_"
                + std::to_string(mpd_id) + "_"
                + std::to_string(adc_ch);
            offset_hist[i] = new TH1I(name.c_str(), "Offset", 1600, 400, 2000);
        }
        if(noise_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_NOISE_"
                + std::to_string(crate_id) + "_"
                + std::to_string(mpd_id) + "_"
                + std::to_string(adc_ch);
            noise_hist[i] = new TH1I(name.c_str(), "Noise", 1600, -800, 800);
        }
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// reset ped histograms

void GEMAPV::ResetPedHist()
{
#ifdef USE_VEC
    // using vector instead of using TH1I
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        offset_vec[i].clear();
        noise_vec[i].clear();
    }
#else
    // obsolete
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if(offset_hist[i])
            offset_hist[i]->Reset();
        if(noise_hist[i])
            noise_hist[i]->Reset();
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// release the memory for histograms

void GEMAPV::ReleasePedHist()
{
#ifdef USE_VEC
    return;
#else
    // obsolete, histos should never be initialized
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        delete offset_hist[i], offset_hist[i] = nullptr;
        delete noise_hist[i], noise_hist[i] = nullptr;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// set time samples and reserve memory for raw data

void GEMAPV::SetTimeSample(const uint32_t &t)
{
    time_samples = t;
    buffer_size = t * MPD_APV_TS_LEN;

    // reallocate the memory for proper size
    delete[] raw_data;

    raw_data = new float[buffer_size];

    ClearData();
}

////////////////////////////////////////////////////////////////////////////////
// clear all the data

void GEMAPV::ClearData()
{
    // set to a high value that won't trigger zero suppression
    for(uint32_t i = 0; i < buffer_size; ++i)
        raw_data[i] = 5000.;

    ResetHitPos();

    commonModeDist.clear();

    for(auto &i: offset_vec)
        i.clear();
    for(auto &i: noise_vec)
        i.clear();
}

////////////////////////////////////////////////////////////////////////////////
// reset hit position array

void GEMAPV::ResetHitPos()
{
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
        hit_pos[i] = false;
}

////////////////////////////////////////////////////////////////////////////////
// clear all the pedestal

void GEMAPV::ClearPedestal()
{
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
        pedestal[i] = Pedestal(0, 0);
}

////////////////////////////////////////////////////////////////////////////////
// update pedestal

void GEMAPV::UpdatePedestal(std::vector<Pedestal> &ped)
{
    for(uint32_t i = 0; (i < ped.size()) && (i < APV_STRIP_SIZE); ++i)
        pedestal[i] = ped[i];
}

////////////////////////////////////////////////////////////////////////////////
// update single channel pedestal

void GEMAPV::UpdatePedestal(const Pedestal &ped, const uint32_t &index)
{
    if(index >= APV_STRIP_SIZE)
        return;

    pedestal[index] = ped;
}

////////////////////////////////////////////////////////////////////////////////
// update single channel pedestal

void GEMAPV::UpdatePedestal(const float &offset, const float &noise, const uint32_t &index)
{
    if(index >= APV_STRIP_SIZE)
        return;

    pedestal[index].offset = offset;
    pedestal[index].noise = noise;
}

////////////////////////////////////////////////////////////////////////////////
// update common mode range for this APV

void GEMAPV::UpdateCommonModeRange(const float &c_min, const float &c_max)
{
    if(c_min >= c_max)
        return;

    common_mode_range_min = c_min;
    common_mode_range_max = c_max;
}

////////////////////////////////////////////////////////////////////////////////
// split data word to adc values. Note that the endianness is changed
// this is for SRS, this should not be used in MPD system

inline void split_data(const uint32_t &data, float &val1, float &val2)
{
    union
    {
        uint8_t bytes[4];
        uint16_t vals[2];
    } word;

    const uint8_t *dbyte = (const uint8_t*) &data;
    for(int i = 0; i < 4; ++i)
    {
        word.bytes[i] = dbyte[3 - i];
    }

    val1 = static_cast<float>(word.vals[0]);
    val2 = static_cast<float>(word.vals[1]);
}

////////////////////////////////////////////////////////////////////////////////
// fill raw data
// this is for SRS. In SRS, each 32-bit word contains two 16-bit words, each 
// 16-bit word corresponds to one ADC value

void GEMAPV::FillRawDataSRS(const uint32_t *buf, const uint32_t &size)
{
    if(2*size > buffer_size) {
        std::cerr << __PRETTY_FUNCTION__ << " Received " << size * 2 << " adc words, "
            << "but APV " << adc_ch << " in MPD " << mpd_id
            << " has only " << buffer_size << " channels" << std::endl;
        return;
    }

    // split 1 32-bit word into 2 16-bit ADC values/
    for(uint32_t i = 0; i < size; ++i)
    {
        split_data(buf[i], raw_data[2*i], raw_data[2*i+1]);
    }

    ts_begin = getTimeSampleStart();
}
////////////////////////////////////////////////////////////////////////////////
// fill raw data
// this is for SRS.

void GEMAPV::FillRawDataSRS(const std::vector<int> &buf, const APVDataType &flags)
{
    if(buf.size() > buffer_size) {
        std::cerr << __PRETTY_FUNCTION__ << " Received " << buf.size() << " adc words, "
            << "but APV " << adc_ch << " in FEC " << mpd_id
            << " has only " << buffer_size << " channels" << std::endl;
        return;
    }

    for(uint32_t i = 0; i < buf.size(); ++i)
    {
        raw_data[i] = static_cast<float>(buf[i]);
    }

    ts_begin = getTimeSampleStart();

    // set raw data flags
    raw_data_flags = flags;
}

////////////////////////////////////////////////////////////////////////////////
// fill raw data
// this is for MPD.

void GEMAPV::FillRawDataMPD(const std::vector<int> &buf, const APVDataType &flags)
{
    if(buf.size() > buffer_size) {
        std::cerr << __PRETTY_FUNCTION__ << " Received " << buf.size() << " adc words, "
            << "but APV " << adc_ch << " in MPD " << mpd_id
            << " has only " << buffer_size << " channels" << std::endl;
        return;
    }

    for(uint32_t i = 0; i < buf.size(); ++i)
    {
        raw_data[i] = static_cast<float>(buf[i]);
    }

    ts_begin = getTimeSampleStart();

    // set raw data flags
    raw_data_flags = flags;
}

////////////////////////////////////////////////////////////////////////////////
// fill fpga online calculated common mode
// to study the difference between online vs offline common mode

void GEMAPV::FillOnlineCommonMode(const std::vector<int> &cm)
{
    online_common_mode.clear();

    for(auto &i: cm) {
        online_common_mode.push_back(i);
    }
}

////////////////////////////////////////////////////////////////////////////////
// fill zero suppressed data, for one specific time sample bin

void GEMAPV::FillZeroSupData(const uint32_t &ch, const uint32_t &ts, const unsigned short &val)
{
    ts_begin = 0;
    uint32_t idx = DATA_INDEX(ch, ts);
    if(ts >= time_samples ||
            ch >= APV_STRIP_SIZE ||
            idx >= buffer_size)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
            << " channel " << ch << " or time sample " << ts
            << " is not allowed."
            << std::endl;
        return;
    }

    hit_pos[ch] = true;
    raw_data[idx] = val;
}

////////////////////////////////////////////////////////////////////////////////
// fill zero suppressed data (for all time samples)

void GEMAPV::FillZeroSupData(const uint32_t &ch, const std::vector<float> &vals)
{
    ts_begin = 0;

    if(vals.size() != time_samples || ch >= APV_STRIP_SIZE)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
            << " channel " << ch << " or time sample " << vals.size()
            << " is not allowed."
            << std::endl;
        return;
    }

    hit_pos[ch] = true;

    for(uint32_t i = 0; i < vals.size(); ++i)
    {
        uint32_t idx = DATA_INDEX(ch, i);
        raw_data[idx] = vals[i];
    }
}

////////////////////////////////////////////////////////////////////////////////
// fill pedestal histogram

void GEMAPV::FillPedHist()
{
    float average[time_samples];

    for(uint32_t i = 0; i < time_samples; ++i)
    {
        //getAverage(average[i], &raw_data[DATA_INDEX(0, i)]); // original method
        getMiddleAverage(average[i], &raw_data[DATA_INDEX(0, i)]); // original method
    }

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        float ch_average = 0.;
        float noise_average = 0.;
        for(uint32_t j = 0; j < time_samples; ++j)
        {
            ch_average += raw_data[DATA_INDEX(i, j)];
            noise_average += raw_data[DATA_INDEX(i, j)] - average[j];
        }
#ifdef USE_VEC
        offset_vec[i].push_back(ch_average/time_samples);
        noise_vec[i].push_back(noise_average/time_samples);
#else
        // obsolete
        if(offset_hist[i])
            offset_hist[i]->Fill(ch_average/time_samples);

        // obsolete
        if(noise_hist[i])
            noise_hist[i]->Fill(noise_average/time_samples);
#endif
    }

    // save common mode
    commonModeDist.insert(commonModeDist.end(), average, average + time_samples);
}

////////////////////////////////////////////////////////////////////////////////
// fit pedestal histogram

void GEMAPV::FitPedestal()
{
#ifdef USE_VEC
    // a helper lambda
    auto fit_vector = [&](const std::vector<int>& vec, double &mean, double &sigma)
    {
        TH1F h("h", "h", 4000, -2000, 2000);

        for(auto &i: vec)
            h.Fill((float)i);

        if(h.GetEntries() > 0) {
            // 1) MPD is too noisy, fitting method is always giving odd result
            //h.Fit("gaus", "Q0");
            //TF1 *myfit = (TF1*)h.GetFunction("gaus");
            //mean = myfit -> GetParameter(1);
            //sigma = myfit -> GetParameter(2);

            // 2) use rms value instead
            mean = h.GetMean();
            sigma = h.GetRMS();
        }
    };

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        // 1) SRS version (used for PRad)
        //double mean = 0, sigma = 5000;
        //fit_vector(offset_vec[i], mean, sigma);
        //double p0 = mean;
        //mean = 0, sigma = 5000;
        //fit_vector(noise_vec[i], mean, sigma);
        //double p1 = sigma;

        // 2) MPD version (used for SSP online suppression)
        double mean = 0, sigma = 5000;
        fit_vector(noise_vec[i], mean, sigma);
        double p0 = mean, p1 = sigma;
        
        UpdatePedestal((float)p0, (float)p1, i);
    }
    return;
#else
    // obsolete
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if( (offset_hist[i] == nullptr) ||
                (noise_hist[i] == nullptr) ||
                (offset_hist[i]->Integral() < 100)// ||
                //(noise_hist[i]->Integral() < 1000) 
          )
        {
            continue;
        }

        offset_hist[i]->Fit("gaus", "qww");
        noise_hist[i]->Fit("gaus", "qww");
        TF1 *myfit = (TF1*) offset_hist[i]->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        myfit = (TF1*) noise_hist[i]->GetFunction("gaus");
        double p1 = myfit->GetParameter(2);

        //TFile *_F = new TFile("example.root", "recreate");
        //offset_hist[i] -> Write();
        //noise_hist[i] -> Write();
        //_F->Close();
        //std::cout<<"enter to continue..."<<std::endl;
        //getchar();

        UpdatePedestal((float)p0, (float)p1, i);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// do zero suppression in raw data space

void GEMAPV::ZeroSuppression()
{
    if(plane == nullptr)
    {
        std::cerr << "GEM APV Error: APV "
            << mpd_id << ", " << adc_ch
            << " is not connected to a detector plane, "
            << "cannot handle data without correct mapping."
            << std::endl;
        return;
    }

    if(DATA_INDEX(APV_STRIP_SIZE, time_samples - 1) >= buffer_size)
    {
        std::cout << mpd_id << ", " << adc_ch << "  "
            << "incorrect time sample position: "  << ts_begin
            << " " << buffer_size << " " << time_samples
            << std::endl;
        return;
    }

    offline_common_mode.clear();
    for(uint32_t ts = 0; ts < time_samples; ++ts)
    {
#ifdef USE_SRS
        CommonModeCorrection_SRS(&raw_data[DATA_INDEX(0, ts)], APV_STRIP_SIZE, ts);
#else
        CommonModeCorrection_MPD(&raw_data[DATA_INDEX(0, ts)], APV_STRIP_SIZE, ts);
#endif
    }

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        float average = 0.;
        for(uint32_t j = 0; j < time_samples; ++j)
        {
            average += raw_data[DATA_INDEX(i, j)];
        }
        average /= time_samples;

#ifdef INVERSE_POLARITY_VALID
        if(abs(average) > pedestal[i].noise * zerosup_thres)
#else
        if(average > pedestal[i].noise * zerosup_thres)
#endif
            hit_pos[i] = true;
        else
            hit_pos[i] = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
// collect zero suppressed hit in raw data space, need a container input

void GEMAPV::CollectZeroSupHits(std::vector<GEM_Strip_Data> &hits)
{
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if(!hit_pos[i])
            continue;

        GEM_Strip_Data hit(crate_id, mpd_id, adc_ch, i);
        for(uint32_t j = 0; j < time_samples; ++j)
        {
            hit.values.emplace_back(raw_data[DATA_INDEX(i, j)]);
        }
        hits.emplace_back(hit);
    }
}

////////////////////////////////////////////////////////////////////////////////
// collect zero suppressed hit in raw data space, directly to connected Plane

void GEMAPV::CollectZeroSupHits()
{
    if(plane == nullptr)
        return;

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if(!hit_pos[i])
            continue;

        plane->AddStripHit(strip_map[i].plane,
                GetMaxCharge(i),
                GetMaxTimeBin(i),
                IsCrossTalkStrip(i),
                crate_id,
                mpd_id,
                adc_ch,
                GetRawTSADC(i)
                );
    }
}

////////////////////////////////////////////////////////////////////////////////
// collect raw hit in raw data space, need a container input
// no zero suppression, for converting evio files to ROOT files

void GEMAPV::CollectRawHits(std::vector<GEM_Strip_Data> &hits)
{
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        GEM_Strip_Data hit(crate_id, mpd_id, adc_ch, i);
        for(uint32_t j = 0; j < time_samples; ++j)
        {
            hit.values.emplace_back(raw_data[DATA_INDEX(i, j)]);
        }
        hits.emplace_back(hit);
    }
}

////////////////////////////////////////////////////////////////////////////////
// a helper to find highest 20 adc strips
// binary insert to a sorted vector and keep that vector to a fixed length
// sorting costs too much running time

static void binary_insert_find_high(std::vector<float> &vec, const float &val, size_t start, size_t end)
{
    if(start + 1 == end)
    {
        for(size_t i=0;i<start;i++)
        {
            vec[i] = vec[i+1];
        }
        vec[start] = val;
        return;
    }

    size_t pos = (start + end) / 2;
    if(vec[pos] >= val)
        binary_insert_find_high(vec, val, start, pos);
    else
        binary_insert_find_high(vec, val, pos, end);
}

static void binary_insert_find_low(std::vector<float> &vec, const float &val, size_t start, size_t end)
{
    if(start + 1 == end)
    {
        for(size_t i=0;i<start;i++)
        {
            vec[i] = vec[i+1];
        }
        vec[start] = val;
        return;
    }

    size_t pos = (start + end) / 2;
    if(vec[pos] < val)
        binary_insert_find_low(vec, val, start, pos);
    else
        binary_insert_find_low(vec, val, pos, end);
}


////////////////////////////////////////////////////////////////////////////////
// do common mode correction (bring the signal average to 0): MPD version

#define NUM_HIGH_STRIPS 20
void GEMAPV::CommonModeCorrection_MPD(float *buf, const uint32_t &size, [[maybe_unused]]const uint32_t &ts)
{
    float average = 0;

    //-----------------------------------------------------------
    // According to Ben, once the online_CM is calculated, the offset will be sutracted 
    // from the raw data, event if you don't subtract cm from the raw data
    // because if one wants to calculate cm, one has to subtract offset first, and it is
    // hard to add the offset back to the raw data online
    // So I add back the offset to reproduce the raw data
    //for(uint32_t i=0; i<size; ++i)
    //{
    //    buf[i] = buf[i] + pedestal[i].offset;
    //}

    // SRS method
    //for(uint32_t i = 0; i < size; ++i)
    //{
    //    //buf[i] = pedestal[i].offset - buf[i]; // for SRS, SRS is using negative ADC value
    //    buf[i] = buf[i] - pedestal[i].offset;   // for MPD, MPD is using positive ADC value
    //    // this is more reasonable than Kondo's initial version
    //    if(buf[i] < pedestal[i].noise * common_thres) {
    //        average += buf[i];
    //        count++;
    //    }
    //}

    // online_zero_suppression is an overall switch 
    // (it decides whether raw_data_flags will be used or not)
    //     when online zero suppression is turned on,
    //     raw_data_flag has two states: 1) OnlineCommonModeSubtractioniEnabled &
    //                                   2) OnlineBuildAllSamples
    //if(!online_zero_suppression || TEST_BIT(raw_data_flags, OnlineBuildAllSamples))
    if(!online_zero_suppression)
    {
        // MPD algorithm -- TODO: needs to refine (absolutely)
        for(uint32_t i = 0; i < size; ++i)
        {
            buf[i] = buf[i] - pedestal[i].offset;
        }
    }
#ifdef SORTING_ALGORITHM
    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        average = dynamic_ts_common_mode_sorting(buf, size);
    }
    else {
        std::cout<<"!online_zero_suppression || TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled)"
                 <<std::endl;
    }
#elif defined(DANNING_ALGORITHM)
    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        average = dynamic_ts_common_mode_danning(buf, size);
    }
#else
    std::cout<<"ERROR: must specifiy one common mode calculation method..."<<std::endl;
    exit(0);
#endif

    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        // common mode correction
        for(uint32_t i = 0; i < size; ++i)
        {
            buf[i] -= average;
        }

        // save offline common mode
        offline_common_mode.push_back(average);
    }
}

////////////////////////////////////////////////////////////////////////////////
// do common mode correction (bring the signal average to 0): SRS version
//#include <TFile.h>
void GEMAPV::CommonModeCorrection_SRS(float *buf, const uint32_t &size, [[maybe_unused]]const uint32_t &ts)
{
    //int count = 0;
    float average = 0;

    // debug
    //auto debug_plot_h = [](float *buf, size_t size, const char* key) -> TH1F*
    //{
    //    TH1F *h = new TH1F(key, key, 150, 0, 150);
    //    for(size_t i=0; i<size; i++)
    //        h -> SetBinContent(i+1, buf[i]);
    //    return h;
    //};

    //TH1F *debug_original_h = debug_plot_h(buf, size, "h_original");

    // SRS method
    for(uint32_t i = 0; i < size; ++i)
    {
        //buf[i] = pedestal[i].offset - buf[i]; // for SRS, SRS is using negative ADC value
        buf[i] = buf[i] - pedestal[i].offset;
        // this is more reasonable than Kondo's initial version
        //if(buf[i] < pedestal[i].noise * common_thres) {
        //    average += buf[i];
        //    count++;
        //}
    }

    //TH1F* debug_offset_sub_h = debug_plot_h(buf, size, "h_offset_sub");

#ifdef SORTING_ALGORITHM
    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        average = dynamic_ts_common_mode_sorting(buf, size);
    }
    else {
        std::cout<<"!online_zero_suppression || TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled)"
                 <<std::endl;
    }
#elif defined(DANNING_ALGORITHM)
    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        average = dynamic_ts_common_mode_danning(buf, size);
    }
#else
    std::cout<<"ERROR: must specifiy one common mode calculation method..."<<std::endl;
    exit(0);
#endif

    //std::cout<<"common mode: "<<average<<std::endl;
    if(!online_zero_suppression || !TEST_BIT(raw_data_flags.data_flag, OnlineCommonModeSubtractionEnabled))
    {
        // common mode correction
        for(uint32_t i = 0; i < size; ++i)
        {
#ifdef USE_SRS
            buf[i] = average - buf[i];
#else
            buf[i] -= average;
#endif
        }

        // save offline common mode
        offline_common_mode.push_back(average);
    }

    //TH1F *debug_cm_sub_h = debug_plot_h(buf, size, "h_cm_sub");
    //TFile *f = new TFile("debug.root", "recreate");
    //debug_original_h -> Write();
    //debug_offset_sub_h -> Write();
    //debug_cm_sub_h -> Write();
    //f -> Close();
    //getchar();
}

////////////////////////////////////////////////////////////////////////////////
// calculate dynamic common mode sorting method

float GEMAPV::dynamic_ts_common_mode_sorting(float *buf, const uint32_t &size)
{
    float average = 0.;
    int count = 0;

#ifdef USE_SRS
    // remove the lowest 20 strips for common mode calculation
    std::vector<float> high_adc(NUM_HIGH_STRIPS, 9999.);
#else
    // remove the highest 20 strips for common mode calculation
    std::vector<float> high_adc(NUM_HIGH_STRIPS, -9999.);
#endif

    for(uint32_t i = 0; i < size; ++i)
    {
        average += buf[i];
        count++;
#ifdef USE_SRS
        if(buf[i] <= high_adc.front())
            binary_insert_find_low(high_adc, buf[i], 0, NUM_HIGH_STRIPS);
#else
        if(buf[i] > high_adc[0])
            binary_insert_find_high(high_adc, buf[i], 0, NUM_HIGH_STRIPS);
#endif
    }

    //std::cout<<"debug: average: "<<average/(float)count<<std::endl;
    for(uint32_t i = 0; i < NUM_HIGH_STRIPS; i++)
    {
        average -= high_adc[i];
        count--;
    }

    if(count)
        average /= (float)count;

    return average;
}

////////////////////////////////////////////////////////////////////////////////
// calculate dynamic common mode danning method

float GEMAPV::dynamic_ts_common_mode_danning(float *buf, const uint32_t &size)
{
    float average = 0;
    int count = 0;
    // 1) average A
    float averageA = 0;
    for(uint32_t i=0; i < size; ++i)
    {
        if (buf[i] >= common_mode_range_min && buf[i] <= common_mode_range_max) {
            averageA += buf[i];
            count++;
        }
    }

    // 2) average B
    if(count > 0) {
        averageA /= (float)count;
        count = 0;
        for(uint32_t i=0; i < size; ++i)
        {
            if(buf[i] < averageA + DANNING_ALGORITHM_RMS_THRESHOLD * pedestal[i].noise) {
                average += buf[i];
                count++;
            }
        }

        if(count > 0) {
            average /= (float)count;
        }
    }
    return average;
}

////////////////////////////////////////////////////////////////////////////////
// get the local strip number from strip map

int GEMAPV::GetLocalStripNb(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE) {
        std::cerr << "GEM APV Get Local Strip Error:"
            << " APV " << adc_ch
            << " in MPD " << mpd_id
            << " in crate "<<crate_id
            << " only has " << APV_STRIP_SIZE
            << " channels." << std::endl;
        return -1;
    }

    return (int)strip_map[ch].local;
}

////////////////////////////////////////////////////////////////////////////////
// get the plane strip number from strip map

int GEMAPV::GetPlaneStripNb(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE) {
        std::cerr << "GEM APV Get Plane Strip Error:"
            << " APV " << adc_ch
            << " in MPD " << mpd_id
            << " in crate "<< crate_id
            << " only has " << APV_STRIP_SIZE
            << " channels." << std::endl;
        return -1;
    }

    return strip_map[ch].plane;
}

////////////////////////////////////////////////////////////////////////////////
// mapping the channel to strip number
// this is for SRS, MPD would not work

GEMAPV::StripNb GEMAPV::MapStripPRad(int ch)
{
    StripNb result;

    // calculate local strip mapping
    // APV25 Internal Channel Mapping
    int strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);

    // APV25 Channel to readout strip Mapping
    if((plane->GetType() == GEMPlane::Plane_X) && (plane_index == 11)) {
        if(strip & 1)
            strip = 48 - (strip + 1)/2;
        else
            strip = 48 + strip/2;
    } else {
        if(strip & 1)
            strip = 32 - (strip + 1)/2;
        else
            strip = 32 + strip/2;
    }

    strip &= 0x7f;
    result.local = strip;

    // calculate plane strip mapping
    // reverse strip number by orient
    if(orient != plane->GetOrientation())
        strip = 127 - strip;

    // special APV
    if((plane->GetType() == GEMPlane::Plane_X) && (plane_index == 11)) {
        strip += -16 + APV_STRIP_SIZE * (plane_index - 1);
    } else {
        strip += APV_STRIP_SIZE * plane_index;
    }

    result.plane = strip;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// mapping the channel to strip number
// this is for MPD

GEMAPV::StripNb GEMAPV::MapStripMPD(int ch)
{
    StripNb result;

    // convert apv internal strip channel number to sorted strip number on GEM det
    const std::string& detector_type = plane -> GetDetector() -> GetType();
    //int strip = apv_strip_mapping::mapped_strip[ch];
    int strip = apv_strip_mapping::mapped_strip_arr.at(detector_type)[ch];

    result.local = strip;

    // calculate plane strip mapping
    // reverse strip number by orient
    if(orient == 1)
        strip = 127 - strip;

    // SBS GEM Coord system (standing at the front side, looking to the front side
    //        ----> Y axis
    //        | GEM 0
    //        | GEM 1
    // X axis | GEM 2
    //        | GEM 3
    //        v

    // in one gem layer, all Y planes cover the same range
    // one X plane covers 1/4 of the total layer length
    int N_APVS_PER_PLANE_X = GetPlane() -> GetCapacity(); // nb of apvs on one plane

    if(plane->GetType() == 0)
        strip = ((detector_position * N_APVS_PER_PLANE_X) + plane_index) 
            * APV_STRIP_SIZE + strip;
    else
        strip = plane_index * APV_STRIP_SIZE + strip;

    result.plane = strip;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// print the pedestal information to ofstream

void GEMAPV::PrintOutPedestal(std::ofstream &out)
{
    out << "APV "
        << std::setw(16) << crate_id
        << std::setw(16) << raw_data_flags.slot_id
        << std::setw(16) << mpd_id
        << std::setw(16) << adc_ch
        << std::endl;

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        out << std::setw(16) << i
            << std::setw(16) << std::setprecision(4) << pedestal[i].offset
            << std::setw(16) << std::setprecision(4) << pedestal[i].noise
            << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// print out the common mode range
//
// format:
//  slot_id, fiber_id, apv_id, cModmin, cModmax
// crate_id,   mpd_id, adc_ch, cModmin, cMOdmax

void GEMAPV::PrintOutCommonModeRange(std::ofstream &out)
{
    float min = 0, max = 0;

    if(commonModeDist.size() > 0) {
        min = max = commonModeDist[0];
        for(auto &i: commonModeDist) {
            if(min > i) min = i;
            if(max < i) max = i;
        }
    }

    // follow Ben's suggestion, set all minimal common mode value to 0
    //min = 0;

    out << std::setw(12) << crate_id
        << std::setw(12) << raw_data_flags.slot_id
        << std::setw(12) << mpd_id
        << std::setw(12) << adc_ch
        << std::setw(12) << static_cast<int>(min)
        << std::setw(12) << static_cast<int>(max)
        << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// print out the common mode using analysis format: average, sigma
//
// format:
//  slot_id, fiber_id, apv_id, cModAvg, cModrms
// crate_id,   mpd_id, adc_ch, cModAvg, cModrms

void GEMAPV::PrintOutCommonModeDBAnaFormat(std::ofstream &out)
{
    float avg = 0, rms = 0;

    if(commonModeDist.size() > 0) {
        TH1F h_temp("h_temp", "h_temp", 1500, 0, 1500);
        for(auto &i: commonModeDist)
            h_temp.Fill(i);
        avg = h_temp.GetMean();
        rms = h_temp.GetRMS();
    }

    out << std::setw(12) << crate_id
        << std::setw(12) << raw_data_flags.slot_id
        << std::setw(12) << mpd_id
        << std::setw(12) << adc_ch
        << std::setw(12) << avg
        << std::setw(12) << rms
        << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// return all the existing histograms

std::vector<TH1I *> GEMAPV::GetHistList()
    const
{
    std::vector<TH1I *> hist_list;

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        if(offset_hist[i])
            hist_list.push_back(offset_hist[i]);
        if(noise_hist[i])
            hist_list.push_back(noise_hist[i]);
    }

    return hist_list;
}

////////////////////////////////////////////////////////////////////////////////
// pack all pedestal info into a vector and return

std::vector<GEMAPV::Pedestal> GEMAPV::GetPedestalList()
    const
{
    std::vector<Pedestal> ped_list;
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        ped_list.push_back(pedestal[i]);
    }

    return ped_list;
}

////////////////////////////////////////////////////////////////////////////////
// get max charge in the specified adc channel

float GEMAPV::GetMaxCharge(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return 0.;

    float val = 0.;
    for(uint32_t j = 0; j < time_samples; ++j)
    {
        float this_val = raw_data[DATA_INDEX(ch, j)];
        if(val < this_val)
            val = this_val;
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
// get max time bin in the specified adc channel

short GEMAPV::GetMaxTimeBin(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return -1;

    float val = 0; short res = -1;
    for(uint32_t j = 0; j < time_samples; ++j)
    {
        float this_val = raw_data[DATA_INDEX(ch, j)];
        if(val < this_val) {
            val = this_val; res = j;
        }
    }

    return res;
}

////////////////////////////////////////////////////////////////////////////////
// get integrated charge in the specified adc channel

float GEMAPV::GetIntegratedCharge(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return 0.;

    float val = 0.;
    for(uint32_t j = 0; j < time_samples; ++j)
    {
        val += raw_data[DATA_INDEX(ch, j)];
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
// get averaged charge in the specified adc channel

float GEMAPV::GetAveragedCharge(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return 0.;

    float val = 0.;
    for(uint32_t j = 0; j < time_samples; ++j)
    {
        val += raw_data[DATA_INDEX(ch, j)];
    }

    return val/time_samples;
}

////////////////////////////////////////////////////////////////////////////////
// get raw time sample adc in the specified adc channel

std::vector<float> GEMAPV::GetRawTSADC(const uint32_t &ch)
    const
{
    std::vector<float> res;

    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return res;

    for(uint32_t j = 0; j < time_samples; ++j)
    {
        float this_val = raw_data[DATA_INDEX(ch, j)];
        res.push_back(this_val);
    }

    return res;
}

////////////////////////////////////////////////////////////////////////////////
// check if this strip is a cross-talk strip (possibly)

bool GEMAPV::IsCrossTalkStrip(const uint32_t &ch)
    const
{
    if(ch >= APV_STRIP_SIZE || !hit_pos[ch])
        return false;

    float max_charge = GetMaxCharge(ch);
    float pre_max = GetMaxCharge(ch - 1);
    float next_max = GetMaxCharge(ch + 1);

    max_charge *= crosstalk_thres;

    if(max_charge < pre_max || max_charge < next_max)
        return true;

    return false;
}

//============================================================================//
// Private Member Functions                                                   //
//============================================================================//

////////////////////////////////////////////////////////////////////////////////
// Compare the data with header level and find where the time sample data begin

uint32_t GEMAPV::getTimeSampleStart()
{
    // in MPD system, the header information was removed, the first
    // ADC value is always the time-sample start
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// get the average within one time sample

void GEMAPV::getAverage(float &average, const float *buf)
{
    average = 0.;
    int count = 0;

    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        average += buf[i];
        count++;
    }

    average /= (float)count;
}

////////////////////////////////////////////////////////////////////////////////
// get the average within one time sample, remove the highst and lowest 20 strips

void GEMAPV::getMiddleAverage(float &average, const float *buf)
{
    average = 0.;
    int count = 0;

    std::vector<float> arr(buf, buf+APV_STRIP_SIZE);
    std::sort(arr.begin(), arr.end());

    for(uint32_t i = 40; i < APV_STRIP_SIZE-40; ++i)
    {
        average += arr[i];
        count++;
    }

    average /= (float)count;
}


////////////////////////////////////////////////////////////////////////////////
// Build strip map
// both local strip map and plane strip map are related to the connected plane
// thus this function will only be called when the APV is connected to the plane

void GEMAPV::buildStripMap()
{
    for(uint32_t i = 0; i < APV_STRIP_SIZE; ++i)
    {
        strip_map[i] = MapStripMPD(i);
    }
}

