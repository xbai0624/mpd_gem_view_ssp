#include "GEMDataHandler.h"
#include "GEMSystem.h"
#include "GEMException.h"
#include "MPDVMERawEventDecoder.h"
#include "MPDSSPRawEventDecoder.h"
#include "RolStruct.h"
#include "GEMRootHitTree.h"
#include "GEMRootClusterTree.h"
#include "APVStripMapping.h"
#include "hardcode.h"

#include <iostream>
#include <chrono>

////////////////////////////////////////////////////////////////////////////////
// ctor

GEMDataHandler::GEMDataHandler()
    : evio_reader(nullptr), event_parser(nullptr), 
    gem_sys(nullptr), new_event(new EventData), proc_event(new EventData),
    root_hit_tree(nullptr)
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// copy ctor

GEMDataHandler::GEMDataHandler(const GEMDataHandler &that)
    : evio_reader(nullptr), event_parser(nullptr), gem_sys(nullptr), 
    new_event(new EventData(*that.new_event)),
    proc_event(new EventData(*that.proc_event)), root_hit_tree(nullptr)
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// move ctor

GEMDataHandler::GEMDataHandler(GEMDataHandler &&that)
    : evio_reader(nullptr), event_parser(nullptr), 
    gem_sys(nullptr), event_data(std::move(that.event_data)),
    new_event(new EventData(std::move(*that.new_event))),
    proc_event(new EventData(std::move(*that.proc_event))),
    root_hit_tree(nullptr)
{
    // place holder
}


////////////////////////////////////////////////////////////////////////////////
// move ctor

GEMDataHandler::~GEMDataHandler()
{
    delete new_event;
    delete proc_event;
}


////////////////////////////////////////////////////////////////////////////////
// copy assignment

GEMDataHandler &GEMDataHandler::operator=(const GEMDataHandler &rhs)
{
    if(this == &rhs) 
        return *this;

    GEMDataHandler that(rhs);
    *this = std::move(that);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// move ctor

GEMDataHandler &GEMDataHandler::operator=(GEMDataHandler &&rhs)
{
    if(this == &rhs)
        return *this;

    delete new_event;
    delete proc_event;

    new_event = rhs.new_event;
    rhs.new_event = nullptr;
    proc_event = rhs.proc_event;
    rhs.proc_event = nullptr;
    event_data = std::move(rhs.event_data);

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// decode

void GEMDataHandler::Decode([[maybe_unused]]const void *buffer)
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// replay event
// functions with '_test' suffix are going to be removed

void GEMDataHandler::ReplayEvent_test(const uint32_t *pBuf, const uint32_t &fBufLen, 
        const int &ev_number)
{
    event_parser -> ParseEvent(pBuf, fBufLen);
#ifdef USE_VME
    MPDVMERawEventDecoder* decoder = dynamic_cast<MPDVMERawEventDecoder*>(
            event_parser->GetRawDecoder(static_cast<int>(Bank_TagID::MPD_VME)) 
            );
#else
    MPDSSPRawEventDecoder* decoder = dynamic_cast<MPDSSPRawEventDecoder*>(
            event_parser->GetRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP)) 
            );
#endif
    const std::unordered_map<APVAddress, std::vector<int>> & decoded_data 
        = decoder->GetAPV();
    const std::unordered_map<APVAddress, APVDataType> & decoded_data_flags
        = decoder -> GetAPVDataFlags();
    const std::unordered_map<APVAddress, std::vector<int>> &decoded_online_cm
        = decoder -> GetAPVOnlineCommonMode();

#ifdef MULTI_THREAD
    const auto & apvs = apv_strip_mapping::Mapping::Instance() -> GetAPVAddressVec();

    // batch process apvs
    auto batch_process_apvs = [&](const size_t &start, const size_t &end)
    {
        for(size_t i=start; i<end; ++i)
        {
            if(gem_sys -> GetAPV(apvs[i]) == nullptr) {
                std::cout<<"Warning:: apv: "<<apvs[i]<<" not initialized."<<std::endl
                    <<"          make sure the correct mapping file was loaded."<<std::endl
                    <<"          skipped the current APV data."<<std::endl;
                continue;
            }
            if(decoded_data.find(apvs[i]) != decoded_data.end()) {
                if(decoded_online_cm.find(apvs[i]) != decoded_online_cm.end())
                    FeedDataMPD(apvs[i], decoded_data.at(apvs[i]), decoded_data_flags.at(apvs[i]),
                            decoded_online_cm.at(apvs[i]));
                else
                    FeedDataMPD(apvs[i], decoded_data.at(apvs[i]), decoded_data_flags.at(apvs[i]));
            }
        }
    };

    // use 4 threads
    size_t NAPVs = apvs.size();
    size_t batch[5] = {0, NAPVs/4, NAPVs/2, NAPVs/4*3, NAPVs};
    std::thread th[4];
    for(int i=0; i<4; ++i) {
        th[i] = std::thread(batch_process_apvs, batch[i], batch[i+1]);
    }
    for(int i=0; i<4; ++i)
        th[i].join();
#else
    for(auto &i: decoded_data)
    {
        if(gem_sys->GetAPV(i.first) == nullptr) {
            std::cout<<"Warning:: apv: "<<i.first<<" not initialized."<<std::endl
                     <<"          make sure the correct mapping file was loaded."<<std::endl
                     <<"          skipped the current APV data."<<std::endl;
            continue;
        }

        if(decoded_online_cm.find(i.first) != decoded_online_cm.end())
            FeedDataMPD(i.first, i.second, decoded_data_flags.at(i.first), decoded_online_cm.at(i.first));
        else
            FeedDataMPD(i.first, i.second, decoded_data_flags.at(i.first));
    }
#endif

    EndofThisEvent(ev_number);
}

////////////////////////////////////////////////////////////////////////////////
// read from single evio file

int GEMDataHandler::ReadFromEvio(const std::string &path, [[maybe_unused]]int split, 
        [[maybe_unused]]bool verbose)
{
    // open evio file
    if(evio_reader != nullptr) {
        evio_reader->CloseFile();
    } else {
        evio_reader = new EvioFileReader();
    }
    evio_reader -> SetFile(path.c_str());

    // open evio file
    bool status = evio_reader -> OpenFile();
    // failed openning file
    if(!status) {
        std::cout<<"Skipped file: "<<path<<std::endl;
        return 0;
    }

    // setup event parser
    if(event_parser != nullptr)
        event_parser -> Reset();
    else
        event_parser = new EventParser();
#ifdef USE_VME
    // setup raw event decoder
    if(mpd_vme_decoder == nullptr) {
        mpd_vme_decoder = new MPDVMERawEventDecoder();

        // register all raw decoders
        event_parser -> RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_VME), mpd_vme_decoder);
    }
#else
    // setup raw event decoder
    if(mpd_ssp_decoder == nullptr) {
        mpd_ssp_decoder = new MPDSSPRawEventDecoder();

        // register all raw decoders
        event_parser -> RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP), mpd_ssp_decoder);
    }
#endif

    // parse event
    int count = 0;
    const uint32_t *pBuf;
    uint32_t fBufLen;
    while(evio_reader -> ReadNoCopy(&pBuf, &fBufLen) == S_SUCCESS)
    {
        count++; // event number in current split evio file

        fEventNumber++; // event number in current run

        ReplayEvent_test(pBuf, fBufLen, fEventNumber);

        if(pedestalMode)
        {
            if(fEventNumber > 5000) // pedestal mode only need 5000 events
                break;
        }
    }

    // wait for end process
    waitEventProcess();

    return count;
} 

////////////////////////////////////////////////////////////////////////////////
// read from splitted evio file

int GEMDataHandler::ReadFromSplitEvio(const std::string &path, int split_start, 
        int split_end, bool verbose)
{
    if(split_end < 0) { // default input, no split
        return ReadFromEvio(path.c_str(), -1, verbose);
    } else {
        int count = 0;
        for(int i=split_start;i<split_end;i++)
        {
            // parse all input files
            size_t pos = 0;
            if(path.find("evio") != std::string::npos) {
                pos = path.find("evio") + 4;
            }
            else if(path.find("dat") != std::string::npos) {
                pos = path.find("dat") + 3;
            }
            else 
            {
                std::cout<<__func__<<" Error: only evio/dat files are accepted."
                         <<path << std::endl;
                return count;
            }
            std::string split_path = path.substr(0, pos);
            split_path = split_path + "." + std::to_string(i);
            count += ReadFromEvio(split_path.c_str(), -1, verbose);
        }
        return count;
    }
}

////////////////////////////////////////////////////////////////////////////////
// replay the raw data file, do zero suppression and save it to root format

void GEMDataHandler::Replay(const std::string &r_path, int split_start, int split_end,
        const std::string &_pedestal_input, const std::string &_common_mode_input,
        const std::string &_pedestal_output, const std::string &_commonMode_output)
{
    Reset();
    // get time start
    std::cout<<std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // set mode before work starts
    SetMode();

    if(replayMode) {
        std::cout<<"INFO::Loading pedestal from : "<<_pedestal_input<<std::endl;
        std::cout<<"INFO::Loading common mode from : "<<_common_mode_input<<std::endl;
        gem_sys -> ReadPedestalFile(_pedestal_input, _common_mode_input);
        // parse output path
        //replay_hit_output_file = ParseOutputFileName(r_path, "Rootfiles/hit_"+std::to_string(split_start));
        //replay_cluster_output_file = ParseOutputFileName(r_path, "Rootfiles/cluster_"+std::to_string(split_start));
        std::string _prefix = "Rootfiles/hit_" + std::to_string(split_start);
        replay_hit_output_file = ParseOutputFileName(r_path, _prefix.c_str());
        _prefix = "Rootfiles/cluster_" + std::to_string(split_start);
        replay_cluster_output_file = ParseOutputFileName(r_path, _prefix.c_str());
        std::cout<<"Replay started..."<<std::endl;
    }
    if(pedestalMode) {
        std::cout<<"Pedestal started..."<<std::endl;
        if(_pedestal_output.size() > 0) pedestal_output_file = _pedestal_output;
        else std::cout<<"Warning: no pedestal output path specified, using default."<<std::endl;
        if(_commonMode_output.size() > 0) commonMode_output_file = _commonMode_output;
        else std::cout<<"Warning: no common mode output path specified, using default."<<std::endl;
    }
    if(onlineMode)
        std::cout<<"Online started..."<<std::endl;

    int count = ReadFromSplitEvio(r_path, split_start, split_end);

    if(replayMode) {
        // save replay root tree
        if(!bReplayCluster) {
            if(root_hit_tree != nullptr)
                root_hit_tree -> Write();    // gem hit tree
        }
        else {
            if(root_cluster_tree != nullptr)
                root_cluster_tree -> Write();// gem cluster tree
        }
    }
    else if(pedestalMode) {
        // save pedestal
        gem_sys -> FitPedestal();
        std::cout<<"saving pedestal file to : "<<pedestal_output_file<<std::endl;
        gem_sys -> SavePedestal(pedestal_output_file.c_str());
        // save common mode range
        std::cout<<"saving commonMode file to : "<<commonMode_output_file<<std::endl;
        gem_sys -> SaveCommonModeRange(commonMode_output_file.c_str());
    }

    // get time end
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    int _t = (int)std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    std::cout<<"Replayed "<<count<<" events";
    std::cout<<" in "<< _t/60 <<" minutes "<<_t%60 <<" seconds"<<std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// clear, erase the data containter and all the connected systems

void GEMDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    event_data = std::deque<EventData>();
    event_parser -> SetEventNumber(0);

    if(gem_sys)
        gem_sys->Reset();
}

////////////////////////////////////////////////////////////////////////////////
// start of new event

void GEMDataHandler::StartOfNewEvent(const unsigned char &tag)
{
    new_event -> update_type(tag);
}

////////////////////////////////////////////////////////////////////////////////
// end of this event

void GEMDataHandler::EndofThisEvent(const int &ev)
{
    new_event -> event_number = ev;

    // wait for the process thread
    waitEventProcess();

    // swap pointers
    EventData *tmp = new_event;
    new_event = proc_event;
    proc_event = tmp;

    end_thread = std::thread(&GEMDataHandler::EndProcess, this, proc_event);
}


////////////////////////////////////////////////////////////////////////////////
// wait for the end process to finish

inline void GEMDataHandler::waitEventProcess()
{
    if(end_thread.joinable())
        end_thread.join();
}

////////////////////////////////////////////////////////////////////////////////
// end process

void GEMDataHandler::EndProcess(EventData *ev)
{
    FillHistograms(*ev);

    // online mode only saves the last event, to reduce usage of memory
    if(onlineMode && event_data.size())
        event_data.pop_front();

    if(replayMode) {
        if(root_hit_tree == nullptr && !bReplayCluster) {
            root_hit_tree = new GEMRootHitTree(replay_hit_output_file.c_str());
        }
        if(root_cluster_tree == nullptr && bReplayCluster) {
            root_cluster_tree = new GEMRootClusterTree(replay_cluster_output_file.c_str());
        }

        if(!bReplayCluster)
            root_hit_tree -> Fill(gem_sys, *ev);
        else {
            // reconstruct clusters
            gem_sys -> Reconstruct(*ev);
            // cluster tree will use gem_sys to extract cluster information
            root_cluster_tree -> Fill(gem_sys, (*ev).event_number);
        }
    }
    else {
        event_data.emplace_back(std::move(*ev)); // save event
    }

    ev->Clear();
}

////////////////////////////////////////////////////////////////////////////////
// Fill histograms

void GEMDataHandler::FillHistograms([[maybe_unused]]const EventData &data)
{
    // to be implemented
}


////////////////////////////////////////////////////////////////////////////////
// feed gem data, for SRS

void GEMDataHandler::FeedDataSRS(const GEMRawData &gemData)
{
    if(gem_sys)
        gem_sys -> FillRawDataSRS(gemData, *new_event);
}

////////////////////////////////////////////////////////////////////////////////
// feed gem data, for MPD

void GEMDataHandler::FeedDataMPD(const APVAddress &addr, const std::vector<int> &raw,
        const APVDataType &flags, const std::vector<int> &online_common_mode)
{
    if(gem_sys)
        gem_sys -> FillRawDataMPD(addr, raw, flags, online_common_mode, *new_event);
}

////////////////////////////////////////////////////////////////////////////////
// feed gem data, for MPD

void GEMDataHandler::FeedDataMPD(const APVAddress &addr, const std::vector<int> &raw,
        const APVDataType &flags)
{
    if(gem_sys)
        gem_sys -> FillRawDataMPD(addr, raw, flags, *new_event);
}

////////////////////////////////////////////////////////////////////////////////
// feed zero sup data

void GEMDataHandler::FeedData(const std::vector<GEMZeroSupData> &gemData)
{
    if(gem_sys)
        gem_sys -> FillZeroSupData(gemData, *new_event);
}


////////////////////////////////////////////////////////////////////////////////
// get event by index

const EventData &GEMDataHandler::GetEvent(const unsigned int &index)
    const
{
    if(!event_data.size())
        throw GEMException("Data Handler Error", "Empty data bank!");

    if(index >= event_data.size()) {
        return event_data.back();
    } else {
        return event_data.at(index);
    }
}


////////////////////////////////////////////////////////////////////////////////
// find event by its event number
// it is assumed the files decoded are all from 1 single run and they are loaded in order
// otherwise this function will not work properly

int GEMDataHandler::FindEvent([[maybe_unused]] int event_number) const
{
    // to be implemented
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// set working mode according to gem system

void GEMDataHandler::SetMode()
{
    if(gem_sys == nullptr)
        return;

    if(gem_sys->GetPedestalMode())
        SetPedestalMode(true);

    else if(gem_sys->GetOnlineMode())
        SetOnlineMode(true);

    else if(gem_sys->GetReplayMode())
        SetReplayMode(true);
}


////////////////////////////////////////////////////////////////////////////////
// reset everything

void GEMDataHandler::Reset()
{
    waitEventProcess();
    fEventNumber = 0;

    if(root_hit_tree != nullptr) {
        delete root_hit_tree;
        // need to NULL root_hit_tree pointer, 
        // delete clause always lead to Undefined Behavior
        root_hit_tree = nullptr;
    }

    if(root_cluster_tree != nullptr) {
        delete root_cluster_tree;
        root_cluster_tree = nullptr;
    }
} 


////////////////////////////////////////////////////////////////////////////////
// automatically generate output file name (based on input file name)
// input file name: xxxx_235.evio.0
//                  xxxx_235.dat.0

std::string GEMDataHandler::ParseOutputFileName(const std::string &input, const char* prefix)
{
    std::string res;
    size_t pos_start = 0;
    if( input.find("evio") != std::string::npos) {
        pos_start = input.find("evio");
    }
    else if(input.find("dat") != std::string::npos) {
        pos_start = input.find("dat");
    }
    else {
        std::cout<<__func__<<" Warning: only evio/dat files are accepted: "<<input
                 <<std::endl;
        return std::string("gem_replay.root");
    }

    size_t not_dir = input.find_last_of("/");
    if(not_dir == std::string::npos)
        not_dir = 0;
    else 
        not_dir += 1;

    res = input.substr(not_dir, pos_start-not_dir);

    res = prefix + std::string("_") + res + "root";

    return res;
}

