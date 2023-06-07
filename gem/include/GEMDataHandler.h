#ifndef GEM_DATA_HANDLER_H
#define GEM_DATA_HANDLER_H

#include <string>
#include <vector>
#include <deque>
#include <thread>
#include "GEMStruct.h"
#include "EventParser.h"
#include "EvioFileReader.h"
#include "GEMAPV.h"

class GEMSystem;
class GEMRootHitTree;
class GEMRootClusterTree;
class MPDVMERawEventDecoder;
class MPDSSPRawEventDecoder;
class SRSRawEventDecoder;
class TriggerDecoder;
struct APVDataType;

class GEMDataHandler
{
public:
    GEMDataHandler();

    // copy/move constructors
    GEMDataHandler(const GEMDataHandler &that);
    GEMDataHandler(GEMDataHandler &&that);

    // destructor
    ~GEMDataHandler();

    // copy/move assignment
    GEMDataHandler &operator=(const GEMDataHandler &rhs);
    GEMDataHandler &operator=(GEMDataHandler &&rhs);

    // set systems
    void SetGEMSystem(GEMSystem *gem){gem_sys = gem;}
    GEMSystem *GetGEMSystem() const {return gem_sys;}
    void SetEvioFileReader(EvioFileReader *e) {evio_reader = e;}
    bool OpenEvioFile(const std::string &path);
    void RegisterRawDecoders();
    int DecodeEvent(int &count);

    // read from multiple evio splits
    int ReadFromSplitEvio(const std::string &path, int split_start = 0,
            int split_end = -1, bool verbose = false);
    // read from single evio
    int ReadFromEvio(const std::string &path, int split=-1, bool verbose = false);
    // interface member
    void Replay(const std::string &r_path, int split_start = 0, int split_end = -1,
            const std::string &pedestal_input_file = "",
            const std::string &common_mode_input_file = "",
            const std::string &pedestal_output_file = "", 
            const std::string &commonMode_output_file = "");
    void SetupReplay(const std::string &r_path, int split_start = 0, int split_end = -1,
            const std::string &pedestal_input_file = "",
            const std::string &common_mode_input_file = "",
            const std::string &pedestal_output_file = "", 
            const std::string &commonMode_output_file = "");
    void Write();
    void Reset();

    // data handler
    void Clear();
    void StartOfNewEvent(const unsigned char &tag);
    void EndofThisEvent(const int &ev);
    void EndProcess(EventData *data);
    void FillHistograms(const EventData &data);

    // feeding data
    void FeedDataSRS(const GEMRawData &gemData);
    // with online cm
    void FeedDataMPD(const APVAddress &addr, const std::vector<int> &raw_data, const APVDataType &flags,
            const std::vector<int> &online_common_mode);
    // online cm not available
    void FeedDataMPD(const APVAddress &addr, const std::vector<int> &raw_data, const APVDataType &flags);
    void FeedData(const std::vector<GEMZeroSupData> &gemData);

    // event storage
    unsigned int GetEventCount() const {return event_data.size();}
    const EventData &GetEvent(const unsigned int &index) const;
    const std::deque<EventData> &GetEventData() const {return event_data;}

    int FindEvent(int event_number) const;
    void ProcessEvent(const uint32_t *pBuf, const uint32_t &fBufLen, const int &ev_number);
    void SetMode();
    void SetPedestalMode(bool m){pedestalMode = m; replayMode = !m; onlineMode = !m;}
    void SetReplayMode(bool m){replayMode = m; pedestalMode = !m; onlineMode = !m;}
    void SetOnlineMode(bool m){onlineMode = m; pedestalMode = !m; onlineMode = !m;}
    void TurnOffClustering(){bReplayCluster = false;}
    void TurnOnClustering(){bReplayCluster = true;}
	void TurnOnbEvio2RootFiles(){bEvio2RootFiles = true;}
	void TurnOffbEvio2RootFiles(){bEvio2RootFiles = false;}
    void EnableOutputRootTree() {root_tree_enabled = true;}
    void DisableOutputRootTree(){root_tree_enabled = false;}
    void SetMaxPedestalEvents(const int &s);

    GEMRootHitTree * GetHitTree() {return root_hit_tree;}
    GEMRootClusterTree *GetClusterTree() {return root_cluster_tree;}
    std::string GetClusterTreeOutputFileName(){return replay_cluster_output_file;}
    std::string GetHitTreeOutputFileName(){return replay_hit_output_file;}

    // helpers
    std::string ParseOutputFileName(const std::string &input_file_name, const char* prefix="");
    void SetOutputPath(const char* str = "");

private:
    void waitEventProcess();

private:
    EvioFileReader *evio_reader;
    EventParser *event_parser;
    GEMSystem *gem_sys;
    std::thread end_thread;
    bool pedestalMode = false;
    bool replayMode = true;
    bool onlineMode = false;

    // decoders
    MPDVMERawEventDecoder *mpd_vme_decoder = nullptr;
    MPDSSPRawEventDecoder *mpd_ssp_decoder = nullptr;
    TriggerDecoder *trigger_decoder = nullptr;
    SRSRawEventDecoder *srs_decoder = nullptr;

    // data related
    std::deque<EventData> event_data;
    EventData *new_event;
    EventData *proc_event;

    // pedestal generate
    std::string pedestal_output_file = "database/gem_ped.dat";
    std::string commonMode_output_file = "database/CommonModeRange.txt";

    int fEventNumber = 0;
    int fMaxPedestalEvents = -1;

    bool root_tree_enabled = true;
    std::string output_path = "Rootfiles/";
    // replay data to root hit tree
    GEMRootHitTree *root_hit_tree = nullptr;
    std::string replay_hit_output_file = "";

    // replay data to root cluster tree
    GEMRootClusterTree *root_cluster_tree = nullptr;
    std::string replay_cluster_output_file = "";
    bool bReplayCluster = false;

    // trigger time
    std::pair<uint32_t, uint32_t> triggerTime;

	// purely convert evio files to root files
	bool bEvio2RootFiles = false;
};

#endif
