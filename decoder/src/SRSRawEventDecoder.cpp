#include "SRSRawEventDecoder.h"

#include <arpa/inet.h>
#include <assert.h>
#include <utility>
#include <fstream>
#include <iostream>

using namespace std;

#define APV_MIN_LENGTH 64

SRSRawEventDecoder::SRSRawEventDecoder()
{
    mAPVRawSingleEvent.clear();
    vActiveADCChannels.clear();
}

SRSRawEventDecoder::SRSRawEventDecoder(unsigned int *buffer, int &n)
{
    mAPVRawSingleEvent.clear();
    vActiveADCChannels.clear();

    Decode(buffer, n);
}

SRSRawEventDecoder::SRSRawEventDecoder(const vector<unsigned int> &buffer)
{
    mAPVRawSingleEvent.clear();
    vActiveADCChannels.clear();

    Decode(buffer);
}

SRSRawEventDecoder::~SRSRawEventDecoder()
{
    Clear();
}

void SRSRawEventDecoder::Clear()
{
    ClearMaps();
    ClearFECMaps();

    vActiveADCChannels.clear();
}

void SRSRawEventDecoder::ClearMaps()
{
    for(auto &i: mAPVRawSingleEvent)
        i.second.clear();
    mAPVRawSingleEvent.clear();
}

void SRSRawEventDecoder::ClearFECMaps()
{
    for(auto &i: mFECAPVEvent)
        i.second.clear();
    mFECAPVEvent.clear();
}

void SRSRawEventDecoder::SwitchEndianess(unsigned int *buf, int &n)
{
    for(int i=0; i<n; ++i)
    {
        buf[i] = ntohl(buf[i]);
    }
}

void SRSRawEventDecoder::Decode(const uint32_t *pBuf, uint32_t fBufLen, std::vector<int> &vTagTrack)
{
    // crate id was passed by upper level ROC id, vTagTrack[1]
    // vTagTrack[0] is current level tag, not crate id
    nCrateID = vTagTrack[1];

    unsigned int *buf = const_cast<unsigned int*>(pBuf);
    int len = fBufLen;

    Decode(buf, len);
}

void SRSRawEventDecoder::Decode(const vector<unsigned int> &buffer)
{
    fBuf = buffer.size();
    unsigned int *buf;
    buf = new unsigned int[fBuf];
    for(int i=0; i<fBuf; ++i)
    {
        buf[i] = buffer[i];
    }

    Decode(buf, fBuf);

    delete[] buf;
}

void SRSRawEventDecoder::DecodeFEC(const vector<unsigned int> &buffer)
{
    fBuf = buffer.size();
    unsigned int *buf;
    buf = new unsigned int[fBuf];

    for(int i=0; i<fBuf; ++i)
        buf[i] = buffer[i];

    DecodeFEC(buf, fBuf);

    delete[] buf;
}

void SRSRawEventDecoder::decode_impl(unsigned int *buf, int &n, vector<int> &apv)
{
    //SwitchEndianess(buf, n);

    bool channel_active = true;

    for(int idata = 0; idata<n; ++idata)
    {
        if(((buf[idata+1] >> 8) & 0xffffff) == 0x414443 )
        {
            if(apv.size() > APV_MIN_LENGTH && channel_active)
            {
                APVAddress addr(nCrateID, nFECID, nADCCh);
                if(mAPVRawSingleEvent.find(addr) == mAPVRawSingleEvent.end())
                {
                    //mAPVRawSingleEvent[addr] = apv;
                    auto tmp = cleanup_srs_apv_header_words(apv);
                    if(tmp.size() == 774)
                        mAPVRawSingleEvent[addr] = tmp;
                    flags.SetAPVAddress(addr);
                    mAPVDataFlags[addr] = flags;
                }
                else {
                    cout<<"decode_impl:: Error: duplicated APV ("<<addr<<") detected..."
                        <<endl;
                }
            }

            apv.clear();
            nADCCh = buf[idata+1] & 0xff;
            nFECID = (buf[idata+2] >> 16) & 0xff;

            idata += 2;
            channel_active = true;

            if(!IsADCChannelActive(nFECID, nADCCh)) {
                channel_active = false;
                CheckInactiveChannel(idata+3, buf);
            }
        }
        else if(buf[idata+1] == 0xfafafafa)
        {
            if(channel_active) {
                FillAPVRaw(apv, buf[idata]);
                if(apv.size() > APV_MIN_LENGTH) {
                    APVAddress addr(nCrateID, nFECID, nADCCh);
                    if(mAPVRawSingleEvent.find(addr) == mAPVRawSingleEvent.end())
                    {
                        //mAPVRawSingleEvent[addr] = apv;
                        auto tmp = cleanup_srs_apv_header_words(apv);
                        if(tmp.size() == 774)
                            mAPVRawSingleEvent[addr] = tmp;
                        flags.SetAPVAddress(addr);
                        mAPVDataFlags[addr] = flags;
                    }
                    else {
                        cout<<"decode_impl:: Error: duplicated APV ("<<addr<<") detected..."
                            <<endl;
                    }
                }
                apv.clear();
            }
            idata += 1;
        }
        else
        {
            if(channel_active)
                FillAPVRaw(apv, buf[idata]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// clean up srs apv header words, make data compatible with mpd setup

std::vector<int> SRSRawEventDecoder::cleanup_srs_apv_header_words(const std::vector<int> &apv,
        const int &TimeSample)
{
    std::vector<int> res;

    //int APV_HEADER = config_setup::apv_header_level;
    //int TimeSample = config_setup::time_sample;

    size_t idata=0; int nTS=0;
    bool start_data_flag = false;
    while(idata < apv.size() && nTS < TimeSample)
    {
        // look for apv header - 3 consecutive words < header
        if(apv[idata] < APV_HEADER) {
            idata++;
            if(apv[idata] < APV_HEADER) {
                idata++;
                if(apv[idata] < APV_HEADER)
                {
                    if(idata + 138 < apv.size()) {
                        idata += 10; // 8 address + 1 error bit
                        start_data_flag = true;
                        nTS++;
                    }
                }
            }
        }

        // meaningful data
        if(start_data_flag) {
            for(int chNo = 0; chNo < 128; ++chNo)
            {
                res.push_back(apv[idata]);
                idata++;
            }
            res.push_back(nTS-1);
            start_data_flag = false;
            continue;
        }
        idata++;
    }

    return res;
}

void SRSRawEventDecoder::Decode(unsigned int *buf, int &n)
{
    if(n <= 0)
        return;
    ClearMaps();
    fBuf = n;

    vector<int> apv;

    decode_impl(buf, n, apv);
}

void SRSRawEventDecoder::DecodeFEC(unsigned int *buf, int &n)
{
    if(n <= 0) return;

    ClearFECMaps();
    fBuf = n;

    vector<int> apv;

    decode_impl(buf, n, apv);
}

void SRSRawEventDecoder::CheckInactiveChannel(int offset, unsigned int *apv_event)
{
    // check first 100 words, to make sure no data lost in case using the wrong mapping
    // by checking if this channel has APV header

    for(int i=offset; i<100; ++i) {
        if(apv_event[i] < APV_HEADER) {
            i++;
            if(apv_event[i] < APV_HEADER) {
                i++;
                if(apv_event[i] < APV_HEADER) {
                    i++;
                    cout<<"## SRS Decoder Warning: ## Found meaningful data in in active channels..."
                        <<endl;
                    cout<<"##  FEC: "<<nFECID<<" Channel: "<<nADCCh<<endl;
                    break;
                }
            }
        }
    }
}

void SRSRawEventDecoder::FillAPVRaw(vector<int> &vec, unsigned int word)
{
    unsigned int word32bit;
    unsigned int word16bit1;
    unsigned int word16bit2;

    word32bit = word;
    Word32ToWord16(&word32bit, &word16bit1, &word16bit2);
    vec.push_back((int)word16bit1);
    vec.push_back((int)word16bit2);
}

unordered_map<APVAddress, vector<int>> & SRSRawEventDecoder::GetDecoded()
{
    return mAPVRawSingleEvent;
}

unordered_map<int, vector<unsigned int>> &SRSRawEventDecoder::GetFECDecoded()
{
    return mFECAPVEvent;
}

const unordered_map<APVAddress, vector<int>> &SRSRawEventDecoder::GetAPV() const
{
    return mAPVRawSingleEvent;
}

const std::unordered_map<APVAddress, APVDataType> & SRSRawEventDecoder::GetAPVDataFlags() const
{
    return mAPVDataFlags;
}

const std::unordered_map<APVAddress, std::vector<int>> & SRSRawEventDecoder::GetAPVOnlineCommonMode() const
{
    return mAPVOnlineCommonMode;
}

std::pair<uint32_t, uint32_t> SRSRawEventDecoder::GetTriggerTime() const
{
    return std::pair<uint32_t, uint32_t>(0, 0);
}

void SRSRawEventDecoder::Word32ToWord16(unsigned int *word, unsigned int *word1, unsigned int *word2)
{
    unsigned int data1=0;
    unsigned int data2=0;
    unsigned int data3=0;
    unsigned int data4=0;

    data1 = ( (*word)>>24 ) & 0xff;
    data2 = ( (*word)>>16 ) & 0xff;
    data3 = ( (*word)>>8  ) & 0xff;
    data4 = (*word) & 0xff;

    (*word1) = (data2 << 8) | data1;
    (*word2) = (data4 << 8) | data3;
}

bool SRSRawEventDecoder::IsADCChannelActive([[maybe_unused]]int &fecid, [[maybe_unused]]int &ch)
{
    return true;
}
