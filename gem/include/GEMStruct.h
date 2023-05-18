#ifndef GEM_STRUCT_H
#define GEM_STRUCT_H

#include <cstdint>
#include <vector>
#include "MPDDataStruct.h"

////////////////////////////////////////////////////////////////
// In mpd apv raw data, the length of one time sample 
// in the unit of uint32_t

#define MPD_APV_TS_LEN 129
#define APV_STRIP_SIZE 128

////////////////////////////////////////////////////////////////
// raw ADC value on a strip (N time samples)

struct StripRawADC
{
    int stripNo;
    std::vector<int> v_adc;

    StripRawADC() : stripNo(-1) {}
    StripRawADC(const StripRawADC &r) = default;
    StripRawADC(StripRawADC &&r) = default;

    int GetTimeSampleSize() const
    {
        return static_cast<int>(v_adc.size());
    }
};

////////////////////////////////////////////////////////////////
// channel (on each APV) address

struct GEMChannelAddress
{
    int crate;
    int mpd;
    int adc;
    int strip;

    GEMChannelAddress() {}
    GEMChannelAddress(const int &c,
            const int &m,
            const int &a,
            const int &s)
        : crate(c), mpd(m), adc(a), strip(s)
    {}
};

////////////////////////////////////////////////////////////////
// gem adc data for each strip

struct GEM_Strip_Data
{
    GEMChannelAddress addr;
    std::vector<float> values;

    GEM_Strip_Data() {}
    GEM_Strip_Data(const int &c,
            const int &m,
            const int &a,
            const int &s)
        : addr(c, m, a, s)
    {}

    void set_address (const int &c,
            const int &m,
            const int &a,
            const int &s)
    {
        addr.crate = c;
        addr.mpd = m;
        addr.adc = a;
        addr.strip = s;
    }

    void add_value(const float &v)
    {
        values.push_back(v);
    }
};

////////////////////////////////////////////////////////////////
// raw event data structure

struct EventData
{
    // event info
    uint32_t event_number;
    uint8_t type;
    uint8_t trigger;
    uint64_t timestamp;

    // data banks
    std::vector<GEM_Strip_Data> gem_data;

    // constructors
    EventData()
        :event_number(0), type(0), trigger(0), timestamp(0)
    {}
    EventData(const uint8_t &t)
        :event_number(0), type(t), trigger(0), timestamp(0)
    {}
    
    void Clear()
    {
        event_number = 0;
        type = 0;
        trigger = 0;
        timestamp = 0;
        gem_data.clear();
    }

    void update_type(const uint8_t &t) {type = t;}
    void update_trigger(const uint8_t &t) {trigger = t;}
    void update_time(const uint64_t &t) {timestamp = t;}

    uint32_t get_type() const {return type;}
    uint32_t get_trigger() const {return trigger;}
    uint64_t get_time() const {return timestamp;}

    void add_gemhit(const GEM_Strip_Data &g) {gem_data.emplace_back(g);}
    void add_gemhit(GEM_Strip_Data &&g) {gem_data.emplace_back(g);}

    std::vector<GEM_Strip_Data> &get_gem_data() {return gem_data;}
    const std::vector<GEM_Strip_Data> &get_gem_data() const {return gem_data;}
};


////////////////////////////////////////////////////////////////
// gem hit struct

struct StripHit
{
    int32_t strip;
    float charge;
    short max_timebin;
    float position;
    bool cross_talk;
    APVAddress apv_addr;
    std::vector<float> ts_adc;

    StripHit()
        : strip(0), charge(0.), max_timebin(-1), position(0.), cross_talk(false), apv_addr(-1, -1, -1)
    { ts_adc.clear(); }

    StripHit(int s, float c, short m, float p, bool f = false, int crate = -1, int mpd = -1, int adc = -1)
        : strip(s), charge(c), max_timebin(m), position(p), cross_talk(f), apv_addr(crate, mpd, adc)
    { ts_adc.clear(); }
};


////////////////////////////////////////////////////////////////
// gem cluster struct 

struct StripCluster
{
    float position;
    float peak_charge;
    short max_timebin;
    float total_charge;
    bool cross_talk;
    std::vector<StripHit> hits;

    StripCluster()
        : position(0.), peak_charge(0.), max_timebin(-1), total_charge(0.), cross_talk(false)
    {}

    StripCluster(const std::vector<StripHit> &p)
        : position(0.), peak_charge(0.), max_timebin(-1), total_charge(0.), cross_talk(false), hits(p)
    {}

    StripCluster(std::vector<StripHit> &&p)
        : position(0.), peak_charge(0.), max_timebin(-1), total_charge(0.), cross_talk(false), hits(std::move(p))
    {}
};

////////////////////////////////////////////////////////////////
// a struct for gem raw data

struct GEMRawData
{
    APVAddress addr;
    const uint32_t *buf;
    uint32_t size;
};

////////////////////////////////////////////////////////////////
// a struct for gem zero suppression data

struct GEMZeroSupData
{
    APVAddress addr;
    int channel;
    int time_sample;
    int adc_value;
};

////////////////////////////////////////////////////////////////
// a base hit information

class BaseHit
{
public:
    float x;            // Cluster's x-position (mm)
    float y;            // Cluster's y-position (mm)
    float z;            // Cluster's z-position (mm)
    float E;            // Cluster's energy (MeV)

    BaseHit()
    : x(0.), y(0.), z(0.), E(0.)
    {}

    BaseHit(float xi, float yi, float zi, float Ei)
    : x(xi), y(yi), z(zi), E(Ei)
    {}
};

////////////////////////////////////////////////////////////////
// gem reconstructed hit

class GEMHit : public BaseHit
{
public:
    int32_t det_id;         // which GEM detector it belongs to
    float x_charge;         // x charge
    float y_charge;         // y charge
    float x_peak;           // x peak charge
    float y_peak;           // y peak charge
    short x_max_timebin;    // x peak time sample
    short y_max_timebin;    // y peak time sample
    int32_t x_size;         // x hits size
    int32_t y_size;         // y hits size
    float sig_pos;          // position resolution

    GEMHit()
    : det_id(-1), x_charge(0.), y_charge(0.), x_peak(0.), y_peak(0.),
      x_max_timebin(-1), y_max_timebin(-1), x_size(0), y_size(0), sig_pos(0.)
    {}

    GEMHit(float xx, float yy, float zz, int d, float xc, float yc,
           float xp, float yp, short x_mt, short y_mt, int xs, int ys, float sig)
    : BaseHit(xx, yy, zz, 0.), det_id(d), x_charge(xc), y_charge(yc),
      x_peak(xp), y_peak(yp), x_max_timebin(x_mt), y_max_timebin(y_mt), 
      x_size(xs), y_size(ys), sig_pos(sig)
    {}
};

// status enums require bitwise manipulation
// the following defintions are copies from Rtypes.h in root (cern)
#define SET_BIT(n,i)  ( (n) |= (1ULL << i) )
#define CLEAR_BIT(n,i)  ( (n) &= ~(1ULL << i) )
#define TEST_BIT(n,i)  ( (bool)( n & (1ULL << i) ) )

// raw data flags
enum APVRawDataFlags
{
    OnlineCommonModeSubtractionEnabled = 0x5, // bit(10 0000); common-mode sub is enabled
    OnlineBuildAllSamples = 0x4,      // bit(01 0000); all samples are recorded(zero-sup disabled)
    OnlineCM_OR = 0x3, // bit(00 1000); common-mode out of range, (cm and zero-sup will be disabled for the following apv frame)
};

#endif
