#ifndef MPD_VME_RAW_EVENT_DECODER_H
#define MPD_VME_RAW_EVENT_DECODER_H

#include "RolStruct.h"
#include "MPDDataStruct.h"
#include "AbstractRawDecoder.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

////////////////////////////////////////////////////////////////
// apv channel data info
// If a 32-bit VME word contains data from apv channel (strip)
// then this struct defines what type of information is stored 
// in that word

enum class APV_Ch_Data_Info
{
    APV_Header  = 0,
    ADC_Value   = 1,
    APV_Trailer = 2,
    Trailer     = 3, // ??
    Undefined
};

////////////////////////////////////////////////////////////////
// structure of mpd vme raw data word
// mpd vme raw data structure, for each 32bit word
// header:            00000000 11100000 00000000 00000000
// mpd id:            00000000 00011111 00000000 00000000
// apv ch data info:  00000000 00011000 00000000 00000000
// adc channel;       00000000 00000000 00000000 00001111
// adc data:          00000000 00000000 00001111 11111111
// apv trailer:       00000000 00000000 00001111 00000000

struct MPD_VME_Raw_Data_Word 
{
    MPD_VME_Raw_Data_Type type;
    int mpd_id;
    APV_Ch_Data_Info apv_ch_data_info;
    int adc_ch;
    int adc;
    int apv_trailer;
    int crate_id;

    MPD_VME_Raw_Data_Word(const uint32_t & word)
    {
        type = 
            static_cast<MPD_VME_Raw_Data_Type>((word & 0x00e00000) >> 21);
        mpd_id = 
            static_cast<int>((word & 0x001f0000) >> 16);
        apv_ch_data_info = 
            static_cast<APV_Ch_Data_Info>((word & 0x00180000) >>19);
        adc_ch = 
            static_cast<int>(word & 0xf);
        adc = 
            static_cast<int>(word & 0xfff);
        apv_trailer = 
            static_cast<int>((word & 0xf00) >> 8);
        crate_id = 
            static_cast<int>(word & 0xff);
    }
};

////////////////////////////////////////////////////////////////
// an auxiliary helper to MPD_VME_Raw_Data_Word
// parse coarse trigger time
typedef struct
{
    uint32_t coarse_trigger_time:20;
    uint32_t trigger_time_index:1;
} coarse_trigger_time_t;

typedef union
{
    uint32_t raw;
    coarse_trigger_time_t trig_time;
} Coarse_Trigger_Time;

// parse fine trigger time
typedef struct
{
    uint32_t fine_trigger_time:8;
    uint32_t n_words_in_event:12;
    uint32_t trailer_leading_bit:1;
} event_trailer_t;

typedef union
{
    uint32_t raw;
    event_trailer_t trailer_word;
} Event_Trailer_Word;



////////////////////////////////////////////////////////////////
// mpd vme raw event decoder

class MPDVMERawEventDecoder : public AbstractRawDecoder
{
public:
    MPDVMERawEventDecoder();
    ~MPDVMERawEventDecoder();

    void Decode(const uint32_t *pBuf, uint32_t fBufLen, 
            std::vector<int> &vTagTrack);

    void DecodeAPV(const uint32_t *pBuf, uint32_t fBufLen,
            std::vector<int> &vTagTrack);

    const std::unordered_map<APVAddress, std::vector<int>> &
        GetAPV() const;

    const std::unordered_map<APVAddress, uint32_t> &
        GetAPVDataFlags() const;

    const std::unordered_map<APVAddress, std::vector<int>> &
        GetAPVOnlineCommonMode() const;

    const std::unordered_map<MPDAddress, uint32_t> &
        GetMPDTiming() const;

    const std::unordered_map<MPDAddress, std::pair<uint64_t, uint32_t>> &
        GetTiming() const;

    std::pair<uint32_t, uint32_t> GetTriggerTime() const;

    void Clear();

private:
    std::unordered_map<APVAddress, std::vector<int>> mAPVData;

    // get timing information, apv clock counts and the timing difference between
    // apv clock and trigger (coarse time, fine time)
    // this is MPD wise (not apv wise)
    // APVs on the same MPD share the same clock and same trigger
    //
    // pair<uint64_t, uint32_t>(APV_clock_counts, Delta(T_Trigger - T_APV_clock))
    std::unordered_map<MPDAddress, std::pair<uint64_t, uint32_t>> mMPDTimingData;

    // apv data flags
    // flags for common mode online done, zero suppression online done
    uint32_t flags = 0;
    std::unordered_map<APVAddress, uint32_t> mAPVDataFlags;
    std::unordered_map<MPDAddress, uint32_t> mMPDTiming;

    // online-calculated common mode
    std::unordered_map<APVAddress, std::vector<int>> mAPVOnlineCommonMode;
};

#endif
