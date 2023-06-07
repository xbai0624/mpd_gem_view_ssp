#ifndef GEM_APV_H
#define GEM_APV_H

#include <vector>
#include <fstream>
#include <iostream>
#include "MPDDataStruct.h"
#include "GEMStruct.h"
#include "MPDSSPRawEventDecoder.h"

class GEMMPD;
class GEMPlane;
class TH1I;

class GEMAPV
{
public:
    struct Pedestal
    {
        float offset;
        float noise;

        // initialize with large noise level so there will be no hits instead
        // of maximum hits when gem is not correctly initialized
        Pedestal() : offset(0.), noise(5000.)
        {}
        Pedestal(const float &o, const float &n)
        : offset(o), noise(n)
        {}
    };

    struct StripNb
    {
        unsigned char local;
        int plane;
    };

public:
    // constrcutor
    GEMAPV(const int &orient,
           const int &det_pos,
           const std::string &status,
           const uint32_t &time_sample = 6,
           const float &common_threshold = 20.,
           const float &zero_threshold = 5.,
           const float &cross_threshold = 8.,
           const bool &fpga_online_zero_suppression = false);

    // copy/move constructors
    GEMAPV(const GEMAPV &p);
    GEMAPV(GEMAPV &&p);

    // destructor
    virtual ~GEMAPV();

    // copy/move assignment operators
    GEMAPV &operator =(const GEMAPV &p);
    GEMAPV &operator =(GEMAPV &&p);

    // member functions
    void ClearData();
    void ClearPedestal();
    void CreatePedHist();
    void ReleasePedHist();
    void FillPedHist();
    void ResetPedHist();
    void FitPedestal();
    void FillRawDataSRS(const uint32_t *buf, const uint32_t &siz);
    void FillRawDataSRS(const std::vector<int> &buf, const APVDataType &flags=APVDataType());
    void FillRawDataMPD(const std::vector<int> &buf, const APVDataType &flags=APVDataType());
    void FillOnlineCommonMode(const std::vector<int> &);
    void FillZeroSupData(const uint32_t &ch, const uint32_t &ts, const unsigned short &val);
    void FillZeroSupData(const uint32_t &ch, const std::vector<float> &vals);
    void UpdatePedestal(std::vector<Pedestal> &ped);
    void UpdatePedestal(const Pedestal &ped, const uint32_t &index);
    void UpdatePedestal(const float &offset, const float &noise, const uint32_t &index);
    void UpdateCommonModeRange(const float &c_min, const float &c_max);
    void ZeroSuppression();
    void CommonModeCorrection_MPD(float *buf, const uint32_t &size, const uint32_t &ts);
    void CommonModeCorrection_SRS(float *buf, const uint32_t &size, const uint32_t &ts);
    float dynamic_ts_common_mode_sorting(float *buf, const uint32_t &size);
    float dynamic_ts_common_mode_danning(float *buf, const uint32_t &size);
    void CollectZeroSupHits(std::vector<GEM_Strip_Data> &hits);
    void CollectZeroSupHits();
    void CollectRawHits(std::vector<GEM_Strip_Data> &hits);
    void ResetHitPos();
    void PrintOutPedestal(std::ofstream &out);
    void PrintOutCommonModeRange(std::ofstream &out);
    void PrintOutCommonModeDBAnaFormat(std::ofstream &out);
    StripNb MapStripPRad(int ch);
    StripNb MapStripMPD(int ch);
    bool IsCrossTalkStrip(const uint32_t &strip) const;

    // get parameters
    int GetMPDID() const {return mpd_id;}
    int GetADCChannel() const {return adc_ch;}
    APVAddress GetAddress() const {return APVAddress(crate_id, mpd_id, adc_ch);}
    uint32_t GetNTimeSamples() const {return time_samples;}
    uint32_t GetTimeSampleSize() const {return APV_STRIP_SIZE;}
    int GetOrientation() const {return orient;}
    int GetPlaneIndex() const {return plane_index;}
    float GetCommonModeThresLevel() const {return common_thres;}
    float GetZeroSupThresLevel() const {return zerosup_thres;}
    float GetCrossTalkThresLevel() const {return crosstalk_thres;}
    uint32_t GetBufferSize() const {return buffer_size;}
    int GetLocalStripNb(const uint32_t &ch) const;
    int GetPlaneStripNb(const uint32_t &ch) const;
    GEMMPD *GetMPD() const {return mpd;}
    GEMPlane *GetPlane() const {return plane;}
    std::vector<TH1I *> GetHistList() const;
    std::vector<Pedestal> GetPedestalList() const;
    float GetMaxCharge(const uint32_t &ch) const;
    short GetMaxTimeBin(const uint32_t &ch) const;
    std::vector<float> GetRawTSADC(const uint32_t &ch) const;
    float GetAveragedCharge(const uint32_t &ch) const;
    float GetIntegratedCharge(const uint32_t &ch) const;
    const std::vector<int> & GetOfflineCommonMode() const {return offline_common_mode;}
    const std::vector<int> & GetOnlineCommonMode() const {return online_common_mode;}

    // set parameters
    void SetMPD(GEMMPD *f, int adc_ch, bool force_set = false);
    void UnsetMPD(bool force_unset = false);
    void SetDetectorPlane(GEMPlane *p, int pl_idx, bool force_set = false);
    void UnsetDetectorPlane(bool force_unset = false);
    void SetTimeSample(const uint32_t &t);
    void SetOrientation(const int &o) {orient = o;}
    void SetCommonModeThresLevel(const float &t) {common_thres = t;}
    void SetZeroSupThresLevel(const float &t) {zerosup_thres = t;}
    void SetCrossTalkThresLevel(const float &t) {crosstalk_thres = t;}
    void SetAddress(const APVAddress &apv_addr);

private:
    void initialize();
    void getAverage(float &ave, const float *buf);
    void getMiddleAverage(float &ave, const float *buf);
    uint32_t getTimeSampleStart();
    void buildStripMap();

private:
    GEMMPD *mpd;
    GEMPlane *plane;
    int crate_id;
    int mpd_id;
    int adc_ch;
    int plane_index; // apv position on the plane [0-11 for X, 0-9 for Y]

    int orient;
    int detector_position; // detector position in GEM layer [0 - 3]

    uint32_t time_samples;
    float common_thres;
    float zerosup_thres;
    float crosstalk_thres;
    bool online_zero_suppression;
    uint32_t buffer_size;
    uint32_t ts_begin;
    float *raw_data;
    Pedestal pedestal[APV_STRIP_SIZE];
    float common_mode_range_min = 0;     // common mode range loaded from file
    float common_mode_range_max = 5000;  // and used for offline analysis
    std::vector<float> commonModeDist;
    StripNb strip_map[APV_STRIP_SIZE];
    bool hit_pos[APV_STRIP_SIZE];

    // TH1I is much slower than vector
    TH1I *offset_hist[APV_STRIP_SIZE];
    TH1I *noise_hist[APV_STRIP_SIZE];
    // use vector for faster process
    std::vector<int> offset_vec[APV_STRIP_SIZE];
    std::vector<int> noise_vec[APV_STRIP_SIZE];

    // raw data flags
    // raw_data_flag.data_flag: lower 6-bit in effect. bit(6)=1: common mode subtracted
    //                          bit(5)=1: build all strips (zero suppression is disabled)
    APVDataType raw_data_flags;

    // common mode calculated offline
    std::vector<int> offline_common_mode;
    // common mode calculated online
    std::vector<int> online_common_mode;
};

#endif
