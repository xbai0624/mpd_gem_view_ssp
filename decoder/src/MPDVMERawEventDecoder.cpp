#include "MPDVMERawEventDecoder.h"
#include "RolStruct.h"

#include <iostream>
#include <bitset>

////////////////////////////////////////////////////////////////
// ctor

MPDVMERawEventDecoder::MPDVMERawEventDecoder()
{
    // place holder
}

////////////////////////////////////////////////////////////////
// dtor

MPDVMERawEventDecoder::~MPDVMERawEventDecoder()
{
    // place holder
}

////////////////////////////////////////////////////////////////
// decode mpd vme raw data

void MPDVMERawEventDecoder::Decode(const uint32_t *pBuf, uint32_t fBufLen,
        std::vector<int> &vTagTrack)
{
    // if you have multi crates, do not use Clear() here, 
    // since each crate will call this function Decode()
    // this function will clear data from previous crates
    // Clear function will be called in EventParser class
    // it is safe to call in there
    // Clear();

    DecodeAPV(pBuf, fBufLen, vTagTrack);
}

////////////////////////////////////////////////////////////////
// decode mpd vme raw data. It seems pretty lengthy, but this 
// DecodeAPV() routine relys on hardware design, It is highly
// possible that it will be changed later

void MPDVMERawEventDecoder::DecodeAPV(const uint32_t *pBuf, uint32_t fBufLen,
        std::vector<int> &vTagTrack)
{
    APVAddress apv_addr; // apv address
    int mpd_id = 0, adc_ch = 0;
    // crate id was was passed by upper level ROC id: vTagTrack[1] (vTagTrack[0] is current level tag)
    int  crate_id = vTagTrack[1]; 

    // apv clock counts (APV 40 MHz clock counts)
    uint64_t apv_clock_counts = 0;

    for(uint32_t i=0;i<fBufLen; i++)
    {
        const uint32_t & data_word = pBuf[i];
        MPD_VME_Raw_Data_Word word(data_word);

        switch(word.type)
        {
            case MPD_VME_Raw_Data_Type::Block_Header:
                mpd_id = word.mpd_id;
                break;
            case MPD_VME_Raw_Data_Type::Block_Trailer:
                break;
            case MPD_VME_Raw_Data_Type::Event_Header:
                break;
            case MPD_VME_Raw_Data_Type::Trigger_Time:
                {
                    Coarse_Trigger_Time _t;
                    _t.raw = data_word;
                    if(_t.trig_time.trigger_time_index == 0) {
                        apv_clock_counts = (_t.trig_time.coarse_trigger_time) << 20;
                    }
                    else if(_t.trig_time.trigger_time_index == 1) {
                        apv_clock_counts |= _t.trig_time.coarse_trigger_time;
                    }
                    else
                        std::cout<<__func__<<" Error in trigger time."<<std::endl;
                }
                break;
            case MPD_VME_Raw_Data_Type::APV_Ch_Data:
                switch(word.apv_ch_data_info)
                {
                    case APV_Ch_Data_Info::APV_Header:
                        {
                            adc_ch = word.adc_ch;
                            APVAddress _ad(crate_id, mpd_id, adc_ch);
                            apv_addr = _ad;
                            mAPVDataFlags[apv_addr] = flags;
                        }
                        break;
                    case APV_Ch_Data_Info::ADC_Value:
                        mAPVData[apv_addr].push_back(word.adc);
                        break;
                    case APV_Ch_Data_Info::APV_Trailer:
                        mAPVData[apv_addr].push_back(word.apv_trailer);
                        break;
                    case APV_Ch_Data_Info::Trailer:
                        break;
                    default:
                        break;
                }
                break;
            case MPD_VME_Raw_Data_Type::Event_Trailer:
                {
                    Event_Trailer_Word _w;
                    _w.raw = data_word;
                    uint32_t fine_trig_time;
                    if(_w.trailer_word.trailer_leading_bit == 0) {
                        fine_trig_time = _w.trailer_word.fine_trigger_time;
                        MPDAddress mpd_addr(crate_id, mpd_id);
                        if(mMPDTimingData.find(mpd_addr) != mMPDTimingData.end())
                        {
                            std::cout<<__func__<<" Error: duplicated mpd detected..."
                                     <<std::endl;
                        }
                        else
                            mMPDTimingData[mpd_addr] =
                                std::pair<uint64_t, uint32_t>(apv_clock_counts, fine_trig_time);
                    }
                    else 
                        std::cout<<__func__<<" Error: fine trigger time error detected"<<std::endl;
                }
                break;
            case MPD_VME_Raw_Data_Type::Crate_Id: 
                break;
            case MPD_VME_Raw_Data_Type::Filler_Word:
                break;
            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////
// get decoded apv data

const std::unordered_map<APVAddress, std::vector<int>> & MPDVMERawEventDecoder::GetAPV() 
    const
{
    return mAPVData;
}

////////////////////////////////////////////////////////////////
// get decoded apv data flags

const std::unordered_map<APVAddress, uint32_t> & MPDVMERawEventDecoder::GetAPVDataFlags() 
    const
{
    return mAPVDataFlags;
}

////////////////////////////////////////////////////////////////
// get decoded mpd timing

const std::unordered_map<MPDAddress, uint32_t> & MPDVMERawEventDecoder::GetMPDTiming() 
    const
{
    return mMPDTiming;
}

////////////////////////////////////////////////////////////////
// get trigger time

std::pair<uint32_t, uint32_t> MPDVMERawEventDecoder::GetTriggerTime() const
{
    return std::pair<uint32_t, uint32_t>(0, 0);
}


////////////////////////////////////////////////////////////////
// get decoded apv data flags

const std::unordered_map<APVAddress, std::vector<int>> & MPDVMERawEventDecoder::GetAPVOnlineCommonMode() 
    const
{
    return mAPVOnlineCommonMode;
}

////////////////////////////////////////////////////////////////
// get decoded timing data

const std::unordered_map<MPDAddress, std::pair<uint64_t, uint32_t>> &
MPDVMERawEventDecoder::GetTiming() const
{
    return mMPDTimingData;
}

////////////////////////////////////////////////////////////////
// clear for next event

void MPDVMERawEventDecoder::Clear() 
{
    mAPVData.clear();
    mAPVDataFlags.clear();
    mAPVOnlineCommonMode.clear();
    mMPDTimingData.clear();
}
