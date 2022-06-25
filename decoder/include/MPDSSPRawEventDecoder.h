#ifndef MPD_SSP_RAW_EVENT_DECODER_H
#define MPD_SSP_RAW_EVENT_DECODER_H

#include <vector>

#include "AbstractRawDecoder.h"
#include "MPDDataStruct.h"
#include "RolStruct.h"

////////////////////////////////////////////////////////////////////////////////
// define macros

#define SSP_TIME_SAMPLE 6 // number of time sample is fixed to 6 in ssp firmware
#define TS_PERIOD_LEN 129 // word length of each time sample

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

////////////////////////////////////////////////////////////////////////////////
// SSP raw data decoder

class MPDSSPRawEventDecoder : public AbstractRawDecoder
{
public:
    MPDSSPRawEventDecoder();
    ~MPDSSPRawEventDecoder();

    void Decode(const uint32_t *pBuf, uint32_t fBufLen, std::vector<int> &vTagTrack);
    void DecodeAPV(const uint32_t *pBuf, uint32_t fBufLen,
            std::vector<int> &vTagTrack);
    const std::unordered_map<APVAddress, std::vector<int>> &
        GetAPV() const;
    const std::unordered_map<APVAddress, APVDataType> &
        GetAPVDataFlags() const;
    const std::unordered_map<APVAddress, std::vector<int>> &
        GetAPVOnlineCommonMode() const;
    const std::unordered_map<MPDAddress, MPDTiming> &
        GetMPDTiming() const;
    std::pair<uint32_t, uint32_t> GetTriggerTime() const;

    void sspApvDataDecode(const uint32_t & data);

    void Clear();

    // debug helper
    void print();

private:
    std::unordered_map<APVAddress, std::vector<int>> mAPVData;

    // flags: lower 6-bit in effect. bit(6)=1: common mode subtracted
    //                               bit(5)=1: build all strips (zero suppression is disabled)
    std::unordered_map<APVAddress, APVDataType> mAPVDataFlags;
    APVAddress apvAddress;

    // common mode calculated online (vector size must be 6)
    std::unordered_map<APVAddress, std::vector<int>> mAPVOnlineCommonMode;
    // apv timing
    std::unordered_map<MPDAddress, MPDTiming> mMPDTiming;

    // the 6 time samples in one strip <channel_no, 6 ADCs>
    uint32_t current_strip_number = -1;
    std::vector<int> vStripADC;
    APVDataType flags;
    MPDTiming mpd_timing;

    uint32_t trigger_time_l = 0;
    uint32_t trigger_time_h = 0;
};

#endif
