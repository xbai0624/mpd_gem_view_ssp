#ifndef GEM_SYSTEM_H
#define GEM_SYSTEM_H

#include <string>
#include <list>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "GEMDetector.h"
#include "GEMMPD.h"
#include "GEMCluster.h"
#include "ConfigObject.h"
#include <mutex>

struct APVDataType;

// mpd id should be consecutive from 0
// enlarge this value if there are more MPDs
#define MAX_MPD_ID 300

// gem detector should be consecutive from 0
// enlarge this value if there are more GEMs
#define MAX_DET_ID 100

// a helper operator to make arguments reading easier
template<typename T>
std::list<ConfigValue> &operator >>(std::list<ConfigValue> &lhs, T &t)
{
    if(lhs.empty()) {
        t = ConfigValue("0").Convert<T>();
    } else {
        t = lhs.front().Convert<T>();
        lhs.pop_front();
    }

    return lhs;
}

// class declare
class GEMSystem : public ConfigObject
{
public:
    // a struct for APV mapping configuration
    struct APV_Entry 
    {
        int crate_id, layer_id, mpd_id;

        // detector id is an unique number assigned to each GEM during assembly
        // it is labeled on each detector
        int detector_id; 

        // x/y plane (0=x; 1=y; to be changed to string)
        int dimension; 
        int adc_ch, i2c_ch, apv_pos, invert;
        std::string discriptor;
        int backplane_id, gem_pos;

        // helper variables
        // name of the detector this apv belongs to
        std::string detector_name;
        // name of the plane this apv belongs to
        std::string plane_name; 
        // size of the plane this apv belongs to
        double plane_size;
        // total number of apvs on the plane this apv belongs to
        int total_connectors;

        // default values, these numbers will be updated by GEMDetectorLayer information
        std::string Plane_Direction[2] = {"X", "Y"};
        double Plane_D[2] = {614.4, 512.0};
        // x plane 12 apvs, y plane 10 apvs
        int Connector_Count[2] = {12, 10};

        APV_Entry(std::list<ConfigValue> entry)
        {
            // order must be correct
            entry >> crate_id >> layer_id >> mpd_id >> detector_id
                >> dimension >> adc_ch >> i2c_ch >> apv_pos >> invert
                >> discriptor >> backplane_id >> gem_pos;

            // default values, these values will be updated by GEMDetectorLayer info
            detector_name = "GEM" + std::to_string(detector_id);
            plane_name = Plane_Direction[dimension];
            plane_size = Plane_D[dimension];
            total_connectors = Connector_Count[dimension];
        }
    };

public:
    // constructor
    GEMSystem(const std::string &config_file = "",
                  int daq_cap = MAX_MPD_ID,
                  int det_cap = MAX_DET_ID);

    // copy/move constructors
    GEMSystem(const GEMSystem &that);
    GEMSystem(GEMSystem &&that);

    // destructor
    virtual ~GEMSystem();

    // copy/move assignment operators
    GEMSystem &operator =(const GEMSystem &rhs);
    GEMSystem &operator =(GEMSystem &&rhs);

    // public member functions
    void RemoveDetector(int det_id);
    void DisconnectDetector(int det_id, bool force_disconn = false);
    void RemoveMPD(const MPDAddress& mpd_addr);
    void DisconnectMPD(const MPDAddress & mpd_addr, bool force_disconn = false);
    void Configure(const std::string &path);
    void ReadMapFile(const std::string &path);
    void ReadPedestalFile(std::string path = "", std::string c_path = "");
    void ReadNoiseAndOffset(const std::string &path);
    void ReadCommonMode(const std::string &path);
    void Clear();
    void ChooseEvent(const EventData &data);
    void Reconstruct();
    void Reconstruct(const EventData &data);
    int GetStripCrossTalkFlag(const GEM_Strip_Data &p, const GEM_Strip_Data &c, const GEM_Strip_Data &n);
    void RebuildDetectorMap();
    void RebuildDAQMap();
    void FillRawDataSRS(const GEMRawData &raw, EventData &event, bool do_zeroSup = true);
    // srs online cm not availabe
    void FillRawDataSRS(const APVAddress &addr, const std::vector<int> &raw,
            const APVDataType &flags, EventData &event, bool do_zeroSup = true);
    // online cm availabe
    void FillRawDataMPD(const APVAddress &addr, const std::vector<int> &raw,
            const APVDataType &flags, const std::vector<int> &online_cm, EventData &event, bool do_zeroSup = true);
    // online cm not available
    void FillRawDataMPD(const APVAddress &addr, const std::vector<int> &raw,
            const APVDataType &flags, EventData &event, bool do_zeroSup = true);
    void FillZeroSupData(const std::vector<GEMZeroSupData> &data_pack, EventData &event);
    void FillZeroSupData(const GEMZeroSupData &data);
    bool Register(GEMDetector *det);
    bool Register(GEMMPD *mpd);

    void SetUnivCommonModeThresLevel(const float &thres);
    void SetUnivZeroSupThresLevel(const float &thres);
    void SetUnivTimeSample(const uint32_t &thres);
    void SetPedestalMode(const bool &m);
    void SetOnlineMode(const bool &m);
    void SetReplayMode(const bool &m);
    void FitPedestal();
    void Reset();
    void SavePedestal(const std::string &path) const;
    void SaveCommonModeRange(const std::string &path) const;
    void SaveHistograms(const std::string &path) const;
    void SetTriggerTime(const std::pair<uint32_t, uint32_t> &);

    GEMCluster *GetClusterMethod() {return &gem_recon;}
    GEMDetector *GetDetector(const int &id) const;
    GEMDetector *GetDetector(const std::string &name) const;
    GEMMPD *GetMPD(const MPDAddress &addr) const;
    GEMAPV *GetAPV(const APVAddress &addr) const;
    GEMAPV *GetAPV(const int &crate_id, const int &mpd, const int &adc) const;

    std::vector<GEM_Strip_Data> GetZeroSupData() const;
    std::vector<GEMAPV*> GetAPVList() const;
    std::vector<GEMMPD*> GetMPDList() const;
    std::vector<GEMDetector*> GetDetectorList() const;
    std::pair<uint32_t, uint32_t> GetTriggerTime() const {return triggerTime;}

    bool GetPedestalMode() const {return PedestalMode;}
    bool GetOnlineMode() const {return OnlineMode;}
    bool GetReplayMode() const {return ReplayMode;}
    void PrintStatus();

private:
    // private member functions
    void buildLayer(std::list<ConfigValue> &layer_args);
    void buildDetector(std::list<ConfigValue> &det_args);
    void buildPlane(std::list<ConfigValue> &pln_args);
    void buildMPD(std::list<ConfigValue> &mpd_args);
    void buildAPV(std::list<ConfigValue> &apv_args);

private:
    GEMCluster gem_recon;
    bool PedestalMode = false;
    bool OnlineMode = false;
    bool ReplayMode = true;

    std::unordered_map<uint32_t, GEMDetectorLayer*> layer_slots;
    std::unordered_map<MPDAddress, GEMMPD*> mpd_slots;
    std::unordered_map<uint32_t, GEMDetector*> det_slots;
    std::unordered_map<std::string, GEMDetector*> det_name_map;

    // default values for creating APV
    unsigned int def_ts;
    float def_cth;
    float def_zth;
    float def_ctth;

    // a locker for multi threading
    std::mutex __gem_locker;

    std::pair<uint32_t, uint32_t> triggerTime;
};

#endif
