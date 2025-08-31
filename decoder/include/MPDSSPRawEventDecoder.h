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
