#ifndef SRS_RAW_EVENT_DECODER_H
#define SRS_RAW_EVENT_DECODER_H

#include <unordered_map>
#include <vector>

#include "MPDDataStruct.h"
#include "AbstractRawDecoder.h"

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
    void FillAPVRaw(std::vector<int> &, unsigned int);

    //std::unordered_map<int, std::unordered_map<int, std::vector<unsigned int>>> &GetDecoded();
    std::unordered_map<APVAddress, std::vector<int>> &GetDecoded();
    std::unordered_map<int, std::vector<unsigned int>> &GetFECDecoded();

    // general interface
    const std::unordered_map<APVAddress, std::vector<int>> &GetAPV() const;

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

    //std::unordered_map<int, std::unordered_map<int, std::vector<unsigned int>>> mAPVRawSingleEvent;
    std::unordered_map<APVAddress, std::vector<int>> mAPVRawSingleEvent;
    std::unordered_map<int, std::vector<unsigned int>> mFECAPVEvent;

    std::vector<int> vActiveADCChannels;
};

#endif
