#ifndef MPD_DATA_STRUCT_H
#define MPD_DATA_STRUCT_H

#include <unordered_map>
#include <ostream>

////////////////////////////////////////////////////////////////
// slot address for MPD

struct MPDAddress
{
    int crate_id;
    int mpd_id;

    // default ctor
    MPDAddress():
        crate_id(0), mpd_id(0)
    {}

    // ctor
    MPDAddress(int cid, int mid):
        crate_id(cid), mpd_id(mid)
    {}

    // copy constructor
    MPDAddress(const MPDAddress &r):
        crate_id(r.crate_id), mpd_id(r.mpd_id)
    {}

    // copy assignment
    MPDAddress& operator=(const MPDAddress& r)
    {
        crate_id = r.crate_id;
        mpd_id = r.mpd_id;
        return *this;
    }

    // equal operator
    bool operator ==(const MPDAddress &r) const
    {
        return (r.crate_id == crate_id) && (r.mpd_id == mpd_id);
    }

    // < operator
    bool operator <(const MPDAddress &a) const
    {
        if(crate_id < a.crate_id)
            return true;
        else if(crate_id == a.crate_id)
        {
            if(mpd_id < a.mpd_id)
                return true;
            else
                return false;
        }
        else 
            return false;
    }

    // > operator
    bool operator >(const MPDAddress &a) const
    {
        if(crate_id > a.crate_id)
            return true;
        else if(crate_id == a.crate_id)
        {
            if(mpd_id > a.mpd_id)
                return true;
            else
                return false;
        }
        else 
            return false;
    }
};

std::ostream &operator<<(std::ostream &, const MPDAddress &addr);


////////////////////////////////////////////////////////////////
// add a hash function for MPDAddress

namespace std {
    template<> struct hash<MPDAddress>
    {
        std::size_t operator()(const MPDAddress &k) const
        {
            return ( ((k.mpd_id & 0x7f)) 
                    | ((k.crate_id & 0xff) << 7)
                   );
        }
    };
}

////////////////////////////////////////////////////////////////
// apv address

struct APVAddress
{
    int crate_id;  // 8 bit
    int mpd_id;    // 7 bit
    int adc_ch;    // 4 bit

    // default ctor
    APVAddress():
        crate_id(0), mpd_id(0), adc_ch(0)
    {}

    // ctor
    APVAddress(int cid, int mid, int aid) :
        crate_id(cid), mpd_id(mid), adc_ch(aid)
    {}

    // copy constructor, should not increase address_id
    APVAddress(const APVAddress & r) : 
        crate_id(r.crate_id), mpd_id(r.mpd_id), adc_ch(r.adc_ch)
    {}

    // copy assignment, should not increase address_id
    APVAddress & operator=(const APVAddress &r)
    {
        crate_id = r.crate_id;
        mpd_id = r.mpd_id;
        adc_ch = r.adc_ch;
        return *this;
    }

    bool operator==(const APVAddress &a) const {
        return (a.crate_id == crate_id) && 
            (a.mpd_id == mpd_id) && (a.adc_ch == adc_ch);
    }

    bool operator<(const APVAddress &a) const 
    {
        if(crate_id < a.crate_id) 
            return true;
        else if(crate_id == a.crate_id)
        {
            if(mpd_id < a.mpd_id) 
                return true;
            else if(mpd_id == a.mpd_id) 
            {
                if(adc_ch < a.adc_ch) 
                    return true;
                else 
                    return false;
            }
            else 
                return false;
        }
        else
            return false;
    }

    bool operator>(const APVAddress &a) const 
    {
        if(crate_id > a.crate_id) 
            return true;
        else if(crate_id == a.crate_id)
        {
            if(mpd_id > a.mpd_id) 
                return true;
            else if(mpd_id == a.mpd_id)
            {
                if(adc_ch > a.adc_ch) 
                    return true;
                else 
                    return false;
            }
            else 
                return false;
        }
        else
            return false;
    }
};

std::ostream &operator <<(std::ostream &os, const APVAddress &ad);

////////////////////////////////////////////////////////////////
// add a hash function for APVAddress

namespace std {
    template<> struct hash<APVAddress>
    {
        std::size_t operator()(const APVAddress &k) const
        {
            return ( (k.adc_ch & 0xf)
                    | ((k.mpd_id & 0x7f) << 4) 
                    | ((k.crate_id & 0xff) << 11)
                   );
        }
    };
}

////////////////////////////////////////////////////////////////
// apv strip address

struct APVStripAddress
{
    int crate_id;     // 8 bit
    int mpd_id;       // 7 bit
    int adc_ch;       // 4 bit
    int strip_no;     // 7 bit

    // default ctor
    APVStripAddress():
        crate_id(0), mpd_id(0), adc_ch(0), strip_no(0)
    {}

    // ctor
    APVStripAddress(int cid, int mid, int aid, int chid) :
        crate_id(cid), mpd_id(mid), adc_ch(aid), strip_no(chid)
    {}

    // ctor
    APVStripAddress(const APVAddress &apv_addr, int stripNo)
    {
        crate_id = apv_addr.crate_id;
        mpd_id = apv_addr.mpd_id;
        adc_ch = apv_addr.adc_ch;
        strip_no = stripNo;
    }

    // copy constructor, should not increase address_id
    APVStripAddress(const APVStripAddress & r) : 
        crate_id(r.crate_id), mpd_id(r.mpd_id), adc_ch(r.adc_ch), strip_no(r.strip_no)
    {}

    // copy assignment, should not increase address_id
    APVStripAddress & operator=(const APVStripAddress &r)
    {
        crate_id = r.crate_id;
        mpd_id = r.mpd_id;
        adc_ch = r.adc_ch;
        strip_no = r.strip_no;
        return *this;
    }

    bool operator == (const APVStripAddress & c) const 
    {
        return ( (c.crate_id == crate_id) && 
                (c.mpd_id == mpd_id) && (c.adc_ch == adc_ch) 
                && (c.strip_no == strip_no) );
    }
};

////////////////////////////////////////////////////////////////
// add a hash function for APVAddress

namespace std {
    template<> struct hash<APVStripAddress>
    {
        std::size_t operator()(const APVStripAddress &k) const
        {
            using std::size_t;
            using std::hash;

            // this should be the ad-hoc method, however 
            // due to the large number of APV channels,
            // this method might introduce collision when
            // crate_id > 0, because if one do a (<< 18)
            // operation, one get at least a number: 2^18
            // = 262144, that is bigger than the default modulus 
            // of underdered_map
            //
            // As a result, one should avoid using
            // APVStripAddress as unordered_map keys
            return ( hash<int>()(k.strip_no & 0x7f)
                    | (hash<int>()(k.adc_ch & 0xf) << 7) 
                    | (hash<int>()(k.mpd_id & 0x7f) << 11)
                    | (hash<int>()(k.crate_id & 0xff) << 18)
                   );
        }
    };
}

////////////////////////////////////////////////////////////////////////////////
// SSP apv data flag

struct APVDataType
{
    uint32_t data_flag;
    uint32_t crate_id;
    uint32_t mpd_id;
    uint32_t adc_ch;
    uint32_t slot_id;

    APVDataType():
        data_flag(0), crate_id(9999), mpd_id(9999), adc_ch(9999), slot_id(11)
    {}

    // copy ctor
    APVDataType(const APVDataType &r):
        data_flag(r.data_flag), crate_id(r.crate_id), mpd_id(r.mpd_id),
        adc_ch(r.adc_ch), slot_id(r.slot_id)
    {}

    // copy assignment
    APVDataType & operator = (const APVDataType &r) {
        data_flag = r.data_flag;
        crate_id = r.crate_id;
        mpd_id = r.mpd_id;
        adc_ch = r.adc_ch;
        slot_id = r.slot_id;
        return *this;
    }

    void SetAPVAddress(const APVAddress &a) {
        crate_id = a.crate_id;
        mpd_id = a.mpd_id;
        adc_ch = a.adc_ch;
    }

    void reset() {
        data_flag = 0;;
        crate_id = 9999;
        mpd_id = 9999;
        adc_ch = 9999;
        slot_id = 11;
    }
};

////////////////////////////////////////////////////////////////////////////////
// mpd timing struct

struct MPDTiming
{
    uint32_t timestamp_coarse1;
    uint32_t timestamp_coarse0;
    uint32_t timestamp_fine;
    uint32_t event_count;

    // default value for timing
    MPDTiming():
        timestamp_coarse1(9999), timestamp_coarse0(9999), 
        timestamp_fine(9999), event_count(0)
    {}

    MPDTiming(const MPDTiming &t):
        timestamp_coarse1(t.timestamp_coarse1), timestamp_coarse0(t.timestamp_coarse0),
        timestamp_fine(t.timestamp_fine), event_count(t.event_count)
    {}

    // copy assignment
    MPDTiming &operator = (const MPDTiming &t) {
        timestamp_coarse1 = t.timestamp_coarse1;
        timestamp_coarse0 = t.timestamp_coarse0;
        timestamp_fine = t.timestamp_fine;
        event_count = t.event_count;
        return *this; 
    }

    // reset
    void reset() {
        timestamp_coarse1 = 9999;
        timestamp_coarse0 = 9999;
        timestamp_fine = 9999;
        event_count = 0;
    }
};

#endif
