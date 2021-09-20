#ifndef GEM_PEDESTAL_H
#define GEM_PEDESTAL_H

#include "MPDDataStruct.h"
#include "GEMStruct.h"
#include "EvioFileReader.h"
#include "EventParser.h"

#include <unordered_map>
#include <vector>
#include <string>

#include <TH1I.h>

class GEMPedestal
{
public:
    GEMPedestal();
    ~GEMPedestal();

    void CalculatePedestal();
    void CalculateEventRawPedestal(const std::unordered_map<APVAddress, std::vector<int>> &);
    void GenerateAPVPedestal_using_histo();
    void GenerateAPVPedestal_using_vec();
    void SetDataFile(const char* path);
    void SetNumberOfEvents(int num);
    void Clear();

    std::vector<StripRawADC> DecodeAPV(std::vector<int> const &);
    std::vector<int> GetTimeSampleCommonMode(const std::vector<StripRawADC> &);

    // helpers
    APVAddress ParseAPVAddressFromString(const std::string &);
    bool APVStripIsNew(const APVStripAddress &);
    void RawAPVUnit_histo(const std::unordered_map<APVAddress, std::vector<int>>::value_type &);
    void RawAPVUnit_vec(const std::unordered_map<APVAddress, std::vector<int>>::value_type &);
    void RawPedestalThread(const std::unordered_map<APVAddress, std::vector<int>> &, int, int);
    void GetEvent(EvioFileReader *, EventParser *, uint32_t &nEvents);
    int GetMean(const std::vector<int> &);
    int GetRMS(const std::vector<int> &);

    // getters
    int GetNumberOfEvents() const;
    const std::unordered_map<APVAddress, std::vector<int>> & GetAPVNoise() const;
    const std::unordered_map<APVAddress, std::vector<int>> & GetAPVOffset() const;
    const std::unordered_map<APVAddress, TH1I*> & GetAPVNoiseHisto() const ;
    const std::unordered_map<APVAddress, TH1I*> & GetAPVOffsetHisto() const;

    // read/write to disk
    void SavePedestalHisto(const char* path);
    void SavePedestalText();
    void LoadPedestalHisto(const char* path);
    void LoadPedestalText();

private:
    std::unordered_map<APVAddress, std::vector<int>> mAPVNoise;
    std::unordered_map<APVAddress, std::vector<int>> mAPVOffset;
    std::unordered_map<APVAddress, TH1I*> mAPVNoiseHisto;
    std::unordered_map<APVAddress, TH1I*> mAPVOffsetHisto;
    // an overall noise distribution (offset is meaningless)
    TH1I *hOverallNoiseHisto;

    // for each strip, they all have a TH1I
    // this might consumes huge memory, seems no way to avoid it
    std::unordered_map<APVStripAddress, TH1I*> mAPVStripNoise;
    std::unordered_map<APVStripAddress, TH1I*> mAPVStripOffset;
    // std::vector is about 3 times faster than TH1I, 
    // for computing intensive jobs, use vector instead of TH1I
    std::unordered_map<APVStripAddress, std::vector<int>> mAPVStripNoiseVec;
    std::unordered_map<APVStripAddress, std::vector<int>> mAPVStripOffsetVec;

    // total number of events used for calculating pedestal
    uint32_t fNumberEvents = 5000;
    std::string data_file_path = "";

    // file reader
    EvioFileReader *file_reader = nullptr;
};

#endif
