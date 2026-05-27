#ifndef SRS_RAW_EVENT_DECODER_H
#define SRS_RAW_EVENT_DECODER_H

#include <unordered_map>
#include <vector>

#include "MPDDataStruct.h"
#include "AbstractRawDecoder.h"

//#define APV_HEADER 1400 // for setup #2
#define APV_HEADER 1700  // for setup #1

class SRSRawEventDecoder : public AbstractRawDecoder
{
public:
    SRSRawEventDecoder();
    SRSRawEventDecoder(const std::vector<unsigned int> &);
    SRSRawEventDecoder(unsigned int*, int &);
    ~SRSRawEventDecoder();

    void SwitchEndianess(unsigned int*, int&);
    void CheckInactiveChannel(int, unsigned int*);
    void Decode(const std::vector<unsigned int> &);
    void Decode(unsigned int*, int&);
    // general interface
    void Decode(const uint32_t* pBuf, uint32_t fBufLen, std::vector<int> &vTagTrack);
    void DecodeFEC(const std::vector<unsigned int> &);
    void DecodeFEC(unsigned int *, int &);
    void decode_impl(unsigned int *buf, int &n, std::vector<int> &apv);
    // Strip the 3-sync + 10-header-word prefix preceding each time sample
    // in an SRS APV frame, returning a vector of (128 channels + 1 TS-id)
    // per time sample. Static so downstream consumers (e.g.
    // GEMAPV::FillRawDataSRS) can call it without needing a decoder
    // instance -- one canonical implementation of the SRS frame layout
    // lives here, by the rest of the SRS parser.
    static std::vector<int> cleanup_srs_apv_header_words(
            const std::vector<int>&, size_t &time_sample);
    void FillAPVRaw(std::vector<int> &, unsigned int);

    //std::unordered_map<int, std::unordered_map<int, std::vector<unsigned int>>> &GetDecoded();
    std::unordered_map<APVAddress, std::vector<int>> &GetDecoded();
    std::unordered_map<int, std::vector<unsigned int>> &GetFECDecoded();

    // general interface. Returns the RAW SRS payload (APV header words still
    // present). Header stripping happens in the downstream consumer
    // (GEMAPV::FillRawDataSRS) so the Viewer can display the raw frame and
    // analysis still gets cleaned data -- one decoder map, one getter.
    const std::unordered_map<APVAddress, std::vector<int>> &GetAPV() const;
    const std::unordered_map<APVAddress, APVDataType> & GetAPVDataFlags() const;
    const std::unordered_map<APVAddress, std::vector<int>> & GetAPVOnlineCommonMode() const;
    std::pair<uint32_t, uint32_t> GetTriggerTime() const;

    void Word32ToWord16(unsigned int*, unsigned int*, unsigned int*);

    bool IsADCChannelActive(int &, int &);
    void Clear();
    void ClearMaps();
    void ClearFECMaps();

private:
    int fBuf = 0;
    int nCrateID = -1;
    int nFECID = -1;
    int nADCCh = -1;
    int APVIndex = -1;
    size_t nTimeSample = 6;

    //std::unordered_map<int, std::unordered_map<int, std::vector<unsigned int>>> mAPVRawSingleEvent;
    std::unordered_map<APVAddress, std::vector<int>> mAPVRawSingleEvent;
    std::unordered_map<int, std::vector<unsigned int>> mFECAPVEvent;

    std::vector<int> vActiveADCChannels;

    // flags for common mode online, not used by SRS
    // flags: lower 6-bit in effect. bit(6)=1: common mode subtracted
    //                               bit(5)=1: build all strips (zero suppression is disabled)
    std::unordered_map<APVAddress, APVDataType> mAPVDataFlags;
    APVAddress apvAddress;
    APVDataType flags;

    // common mode calculated online (vector size must be 6)
    std::unordered_map<APVAddress, std::vector<int>> mAPVOnlineCommonMode;
};

#endif
