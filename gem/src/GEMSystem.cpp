//============================================================================//
// GEM System class                                                           //
// It has both the physical and the DAQ structure of GEM detectors            //
// Physical structure: N detectors, each has 2 planes (X, Y)                  //
// DAQ structure: several MPDs, each has several APVs                         //
// APVs and detector planes are connected                                     //
//                                                                            //
// MPDs and Detectors are directly managed by GEM System                      //
// Each MPD or Detector will manage its own APV or Plane members              //
//                                                                            //
// Framework of the DAQ system (DET-PLN & MPD-APV) are based on the code from //
// Kondo Gnanvo and Xinzhan Bai                                               //
//                                                                            //
// Chao Peng                                                                  //
// 10/29/2016                                                                 //
// Xinzhan Bai, adapt to MPD system                                           //
// 12/02/2020                                                                 //
//============================================================================//

#include <TFile.h>
#include <TH1I.h>
#include <cstdint>
#include "GEMSystem.h"
#include "GEMMPD.h"
#include "GEMDetectorLayer.h"
#include "GEMException.h"

//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

////////////////////////////////////////////////////////////////////////////////
// constructor

GEMSystem::GEMSystem(const std::string &config_file, int mpd_cap, int det_cap)
: PedestalMode(false), def_ts(6), def_cth(20.), def_zth(5.), def_ctth(8.)
{
    mpd_slots.reserve(mpd_cap);
    det_slots.reserve(det_cap);

    if(!config_file.empty())
        Configure(config_file);

    triggerTime.first = 0.;
    triggerTime.second = 0;
}

////////////////////////////////////////////////////////////////////////////////
// the copy and move constructor will not only copy all the members that managed
// by GEM System, but also build the connections between them
// copy constructor, the complicated part is to copy the connections between
// Planes and APVs

GEMSystem::GEMSystem(const GEMSystem &that)
: ConfigObject(that),
  gem_recon(that.gem_recon), PedestalMode(that.PedestalMode),
  def_ts(that.def_ts), def_cth(that.def_cth), def_zth(that.def_zth),
  def_ctth(that.def_ctth), triggerTime(that.triggerTime)
{
    // copy daq system first
    for(auto &mpd : that.mpd_slots)
    {
        if(mpd.second == nullptr)
            mpd_slots.emplace(mpd.first, nullptr);
        else
            mpd_slots.emplace(mpd.first, new GEMMPD(*(mpd.second)));
    }

    // then copy detectors and planes
    for(auto &det : that.det_slots)
    {
        if(det.second == nullptr) {
            det_slots.emplace(det.first, nullptr);
        } else {
            GEMDetector *new_det = new GEMDetector(*(det.second));
            det_slots.emplace(det.first, new_det);

            // copy the connections between APVs and planes
            auto that_planes = det.second->GetPlaneList();
            for(uint32_t i = 0; i < that_planes.size(); ++i)
            {
                auto that_apvs = that_planes[i]->GetAPVList();
                GEMPlane *this_plane = new_det->GetPlaneList().at(i);
                for(auto &apv : that_apvs)
                {
                    GEMAPV *this_apv = GetAPV(apv->GetAddress());
                    this_plane->ConnectAPV(this_apv, apv->GetPlaneIndex());
                }
            }
        }
    }

    RebuildDetectorMap();
}

////////////////////////////////////////////////////////////////////////////////
// move constructor

GEMSystem::GEMSystem(GEMSystem &&that)
: ConfigObject(that),
  gem_recon(std::move(that.gem_recon)), PedestalMode(that.PedestalMode),
  mpd_slots(std::move(that.mpd_slots)), det_slots(std::move(that.det_slots)),
  det_name_map(std::move(that.det_name_map)), def_ts(that.def_ts),
  def_cth(that.def_cth), def_zth(that.def_zth), def_ctth(that.def_ctth), 
  triggerTime(that.triggerTime)
{
    // reset the system for all components
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->SetSystem(this);
    }

    for(auto &det : det_slots)
    {
        if(det.second)
            det.second->SetSystem(this);
    }
}


////////////////////////////////////////////////////////////////////////////////
// desctructor

GEMSystem::~GEMSystem()
{
    Clear();
}


////////////////////////////////////////////////////////////////////////////////
// copy assignment operator

GEMSystem &GEMSystem::operator =(const GEMSystem &rhs)
{
    if(this == &rhs)
        return *this;

    GEMSystem that(rhs); // use copy constructor
    *this = std::move(that); // use move assignment operator
    return *this;
}


////////////////////////////////////////////////////////////////////////////////
// move assignment operator

GEMSystem &GEMSystem::operator =(GEMSystem &&rhs)
{
    if(this == &rhs)
        return *this;

    ConfigObject::operator =(rhs);

    // release current resources
    Clear();

    // move everything here
    gem_recon = std::move(rhs.gem_recon);
    PedestalMode = rhs.PedestalMode;

    mpd_slots = std::move(rhs.mpd_slots);
    det_slots = std::move(rhs.det_slots);
    det_name_map = std::move(rhs.det_name_map);

    def_ts = rhs.def_ts;
    def_cth = rhs.def_cth;
    def_zth = rhs.def_zth;
    def_ctth = rhs.def_ctth;

    // reset the system for all components
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->SetSystem(this);
    }

    for(auto &det : det_slots)
    {
        if(det.second)
            det.second->SetSystem(this);
    }

    return *this;
}

//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

////////////////////////////////////////////////////////////////////////////////
// configure this class

void GEMSystem::Configure(const std::string &path)
{
    bool verbose = false;

    if(!path.empty()) {
        ConfigObject::Configure(path);
        verbose = true;
    }

    CONF_CONN(def_ts, "Default Time Samples", 6, verbose);
    CONF_CONN(def_cth, "Default Common Mode Threshold", 20, verbose);
    CONF_CONN(def_zth, "Default Zero Suppression Threshold", 5, verbose);
    CONF_CONN(def_ctth, "Default Cross Talk Threshold", 8, verbose);

    gem_recon.Configure(Value<std::string>("GEM Cluster Configuration"));

    // read gem map, build DAQ system and detectors
    try {
        ReadMapFile(Value<std::string>("GEM Map"));

        // this has been moved to GEMDataHandler::Replay() function, it is more reasonable to do it there
        //if(!PedestalMode)
        //    ReadPedestalFile(Value<std::string>("GEM Pedestal"), Value<std::string>("GEM Common Mode"));
    } catch(GEMException &e) {
        std::cerr << e.FailureType() << ": "
                  << e.FailureDesc() << std::endl;
    }

    // set resolution for each detector, defalt 0.1
    float def_res = 0.1;
    for(auto &det : det_slots)
    {
        if(det.second == nullptr) continue;
        std::string key = "Position Resolution [" + det.second->GetName() + "]";
        auto val = Value(key);
        if(!val.IsEmpty())
            det.second->SetResolution(val.Double());
        else
            det.second->SetResolution(def_res);
    }

    //PrintStatus();
}

////////////////////////////////////////////////////////////////////////////////
// remove all the components

void GEMSystem::Clear()
{
    // DAQ removal
    for(auto &mpd : mpd_slots)
    {
        // this prevent mpd from calling RemoveMPD upon destruction
        if(mpd.second)
            mpd.second->UnsetSystem(true);
        delete mpd.second;
    }

    // Detectors removal
    for(auto &det : det_slots)
    {
        // this prevent detector from calling removeDetector upon destruction
        if(det.second)
            det.second->UnsetSystem(true);
        delete det.second;
    }

    triggerTime.first = 0;
    triggerTime.second = 0;

    det_name_map.clear();
}


// Read the map file
void GEMSystem::ReadMapFile(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    c_parser.SetSplitters(",");

    if(!c_parser.ReadFile(path)) {
        throw GEMException("GEM System", "cannot open GEM map file " + path);
    }
    std::cout<<__func__<<" Loading map file from: "<<path<<std::endl;

    // release memory before load new configuration
    Clear();

    // we accept 5 types of elements
    // layer, detector, plane, mpd, apv
    std::vector<std::string> types = {"Layer", "DET", "PLN", "MPD", "APV"};
    std::vector<int> expect_args = {12, 3, 6, 2, 8};
    std::vector<int> option_args = {0, 0, 0, 0, 4};
    // this std::vector is to store all the following arguments
    std::vector<std::vector<std::list<ConfigValue>>> args(types.size());

    // read all the elements in
    while(c_parser.ParseLine())
    {
        std::string key = c_parser.TakeFirst();
        uint32_t i = 0;
        for(; i < types.size(); ++i)
        {
            if(ConfigParser::case_ins_equal(key, types.at(i))) {
                if(c_parser.CheckElements(expect_args.at(i), option_args.at(i)))
                    args[i].push_back(c_parser.TakeAll<std::list>());
                break;
            }
        }

        if(i >= types.size()) { // did not find any type
            std::cout << " GEM System Warning: Undefined element type "
                      << key << " in configuration file "
                      << "\"" << path << "\""
                      << std::endl;
        }
    }

    // order is very important,
    // since detectors will be added to layers
    // planes will be added to detectors
    // apvs will be added to mpds and connected to planes 
    // add layers, detectors and planes
    for(auto &layer: args[0])
        buildLayer(layer);
    for(auto &det : args[4])
        buildDetector(det);
    for(auto &pln : args[4])
        buildPlane(pln);

    // add mpds and apvs
    for(auto &mpd : args[4])
        buildMPD(mpd);
    for(auto &apv : args[4])
        buildAPV(apv);

    // Rebuilding the maps just helps sort the lists, so they won't depend on
    // the orders in configuration map
    RebuildDetectorMap();
}

// Load pedestal and common mode files, update all APVs' pedestal and common mode
void GEMSystem::ReadPedestalFile(std::string path, std::string c_path)
{
    // noise + offset
    ReadNoiseAndOffset(path);

    // common mode
    ReadCommonMode(c_path);
}

// Load pedestal file and update all APVs' pedestal
void GEMSystem::ReadNoiseAndOffset(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    c_parser.SetSplitters(",: \t");

    if(!c_parser.ReadFile(path)) {
	    std::cout<<"ERROR GEMSystem::ReadNoiseAndOffset(): cannot open pdestal file: "
		    <<path<<std::endl;
	    std::cout<<"      Using default values. Please check your config file."
		    <<std::endl;
	    return;
        //throw GEMException("GEM System", "cannot open pedestal data file " + path);
    }

    GEMAPV *apv = nullptr;

    while(c_parser.ParseLine())
    {
        ConfigValue first = c_parser.TakeFirst();

        if(first == "APV") { // a new APV
            int crate_id, mpd, adc, slot_id;
            if(c_parser.NbofElements() == 3)
                c_parser >> crate_id >> mpd >> adc;
            else if(c_parser.NbofElements() == 4)
                c_parser >> crate_id >> slot_id >> mpd >> adc;
            else
                std::cout<<"Error: unsupported pedestal file."
                         <<std::endl;

            apv = GetAPV(crate_id, mpd, adc);

            if(apv == nullptr) {
                std::cout << " GEM System Warning: Cannot find APV "
                          << crate_id << ", " << mpd <<  ", " << adc
                          << " , skip updating its pedestal."
                          << std::endl;
            }
        } else { // different adc channel in this APV
            float offset, noise;
            c_parser >> offset >> noise;

            if(apv)
                apv->UpdatePedestal(GEMAPV::Pedestal(offset, noise), first.Int());
            // seems no need to break the program, if found pedestal for apvs not in mapping file, skip it
            //else
            //    throw GEMException("GEM System", "updating pedestal: apv not found");
        }
    }
}

// Load common mode file and update all APVs' common mode
void GEMSystem::ReadCommonMode(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    c_parser.SetSplitters(",: \t");

    if(!c_parser.ReadFile(path)) {
	    std::cout<<"ERROR GEMSystem::ReadCommonMode(): cannot open file: "
		    <<path<<std::endl;
	    std::cout<<"       Using default values. Please check your config file."
		    <<std::endl;
	    return;
        //throw GEMException("GEM System", "cannot open pedestal data file " + path);
    }

    GEMAPV *apv = nullptr;

    while(c_parser.ParseLine())
    {
        int crate_id, mpd, adc, slot_id;
        float min, max;
        if(c_parser.NbofElements() == 5)
            c_parser >> crate_id >> mpd >> adc >> min >> max;
        else if(c_parser.NbofElements() == 6)
            c_parser >> crate_id >> slot_id >> mpd >> adc >> min >> max;
        else
            std::cout<<"Error: Unsupported common mode file."
                     <<std::endl;
 
        apv = GetAPV(crate_id, mpd, adc);

        if(apv == nullptr) {
            std::cout << " GEM System Warning: Cannot find APV "
                << crate_id << ", " << mpd <<  ", " << adc
                << " , skip updating its common mode."
                << std::endl;
            continue;
        }
        else {
            if(apv)
                apv->UpdateCommonModeRange(min, max);
        }
    }
}

// return true if the detector is successfully registered
bool GEMSystem::Register(GEMDetector *det)
{
    if(det == nullptr)
        return false;

    if(det_slots.size() > MAX_DET_ID)
    {
        std::cerr << "GEM System Error: Failed to add detector "
                  << det->GetName()
                  << " (" << det->GetDetID() << ")"
                  << ", exceeds the maximum capacity."
                  << std::endl;
        return false;
    }

    if(det_slots[det->GetDetID()] != nullptr)
    {
        // detector already registered, do nothing
        return false;
    }

    det->SetSystem(this);
    det_slots[det->GetDetID()] = det;
    return true;
}

// return true if the mpd is successfully registered
bool GEMSystem::Register(GEMMPD *mpd)
{
    if(mpd == nullptr)
        return false;
    
    if(mpd_slots.find(mpd->GetAddress()) != mpd_slots.end() && 
            mpd_slots.at(mpd->GetAddress()) != nullptr)
    {
        // already added, do nothing
        return false;
    }

    mpd->SetSystem(this);
    mpd_slots[mpd->GetAddress()] = mpd;
    return true;
}

// disconnect detector, and rebuild the detector map
void GEMSystem::DisconnectDetector(int det_id, bool force_disconn)
{
    if(det_slots.find((uint32_t)det_id) == det_slots.end())
        return;

    auto &det = det_slots[det_id];
    if(!det)
        return;

    if(!force_disconn)
        det->UnsetSystem(true);

    det = nullptr;
    // rebuild maps
    RebuildDetectorMap();
}

// remove detector, and rebuild the detector map
void GEMSystem::RemoveDetector(int det_id)
{
    if(det_slots.find((uint32_t)det_id) == det_slots.end())
        return;

    auto &det = det_slots[det_id];
    if(!det)
        return;

    det->UnsetSystem(true);
    delete det, det = nullptr;

    // rebuild maps
    RebuildDetectorMap();
}

// disconnect MPD, and rebuild the DAQ map
void GEMSystem::DisconnectMPD(const MPDAddress &mpd_addr, bool force_disconn)
{
    if(mpd_slots.find(mpd_addr) == mpd_slots.end())
        return;

    auto &mpd = mpd_slots[mpd_addr];
    if(!mpd)
        return;

    if(!force_disconn)
        mpd->UnsetSystem(true);

    mpd = nullptr;
}

void GEMSystem::RemoveMPD(const MPDAddress &mpd_addr)
{
    if(mpd_slots.find(mpd_addr) == mpd_slots.end())
        return;

    auto &mpd = mpd_slots[mpd_addr];
    if(!mpd)
        return;

    mpd->UnsetSystem(true);
    delete mpd, mpd = nullptr;
}

// rebuild detector related maps
void GEMSystem::RebuildDetectorMap()
{
    det_name_map.clear();

    for(auto &det : det_slots)
    {
        if(det.second == nullptr)
            continue;

        det_name_map[det.second->GetName()] = det.second;
    }
}

// find detector by detector id
GEMDetector *GEMSystem::GetDetector(const int &det_id)
const
{
    if(det_slots.find(static_cast<uint32_t>(det_id)) == det_slots.end()) {
        return nullptr;
    }
    return det_slots.at(det_id);
}

// find detector by its name
GEMDetector *GEMSystem::GetDetector(const std::string &name)
const
{
    auto it = det_name_map.find(name);
    if(it == det_name_map.end()) {
        std::cerr << "GEM System: Cannot find detector " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

// find MPD by its address
GEMMPD *GEMSystem::GetMPD(const MPDAddress &addr)
const
{
    if(mpd_slots.find(addr) != mpd_slots.end())
    {
        return mpd_slots.at(addr);
    }

    return nullptr;
}

// find APV by its MPD id and adc channel
GEMAPV *GEMSystem::GetAPV(const int & crate_id, 
        const int &mpd_id, const int &apv_id) const
{
    MPDAddress addr(crate_id, mpd_id);

    if ((mpd_slots.find(addr) != mpd_slots.end()) &&
            (mpd_slots.at(addr) != nullptr)) {

        return mpd_slots.at(addr)->GetAPV(apv_id);
    }

    return nullptr;
}

// find APV by its ChannelAddress
GEMAPV *GEMSystem::GetAPV(const APVAddress &addr)
const
{
    return GetAPV(addr.crate_id, addr.mpd_id, addr.adc_ch);
}

// fill raw data to a certain apv
void GEMSystem::FillRawDataSRS(const GEMRawData &raw, EventData &event, bool do_zeroSup)
{
    GEMAPV *apv = GetAPV(raw.addr);

    if(apv != nullptr) {

        apv->FillRawDataSRS(raw.buf, raw.size);

        if(PedestalMode)
            apv->FillPedHist();
        else {
			if(do_zeroSup)
				apv->ZeroSuppression();

#ifdef MULTI_THREAD
            __gem_locker.lock();
#endif

			if(do_zeroSup)
				apv->CollectZeroSupHits(event.get_gem_data());
			else
				apv->CollectRawHits(event.get_gem_data());

#ifdef MULTI_THREAD
            __gem_locker.unlock();
#endif
        }
    }
}

// fill raw data to a certain apv, online cm not availabe
void GEMSystem::FillRawDataSRS(const APVAddress &addr, const std::vector<int> &raw,
        const APVDataType &flags, EventData &event, bool do_zeroSup)
{
    GEMAPV *apv = GetAPV(addr);

    if(apv != nullptr) 
    {
        apv->FillRawDataSRS(raw, flags);

        if(PedestalMode)
            apv->FillPedHist();
        else {
			if(do_zeroSup)
				apv->ZeroSuppression();

#ifdef MULTI_THREAD
            __gem_locker.lock();
#endif

			if(do_zeroSup)
				apv->CollectZeroSupHits(event.get_gem_data());
			else
				apv->CollectRawHits(event.get_gem_data());

#ifdef MULTI_THREAD
            __gem_locker.unlock();
#endif
        }
    }
    else 
    {
        std::cout<<__func__<<" waring:: APV "<<addr<<" not found."<<std::endl;
    }
}

// fill raw data to a certain apv, online cm available
void GEMSystem::FillRawDataMPD(const APVAddress &addr, const std::vector<int> &raw,
        const APVDataType &flags, const std::vector<int> &online_cm, EventData &event, bool do_zeroSup)
{
    GEMAPV *apv = GetAPV(addr);

    if(apv != nullptr) 
    {
        apv->FillRawDataMPD(raw, flags);
        apv->FillOnlineCommonMode(online_cm);

        if(PedestalMode)
            apv->FillPedHist();
        else {
			if(do_zeroSup)
				apv->ZeroSuppression();

#ifdef MULTI_THREAD
            __gem_locker.lock();
#endif

			if(do_zeroSup)
				apv->CollectZeroSupHits(event.get_gem_data());
			else
				apv->CollectRawHits(event.get_gem_data());

#ifdef MULTI_THREAD
            __gem_locker.unlock();
#endif
        }
    }
    else 
    {
        std::cout<<__func__<<" waring:: APV "<<addr<<" not found."<<std::endl;
    }
}

// fill raw data to a certain apv, online cm not availabe
void GEMSystem::FillRawDataMPD(const APVAddress &addr, const std::vector<int> &raw,
        const APVDataType &flags, EventData &event, bool do_zeroSup)
{
    GEMAPV *apv = GetAPV(addr);

    if(apv != nullptr) 
    {
        apv->FillRawDataMPD(raw, flags);

        if(PedestalMode)
            apv->FillPedHist();
        else {
			if(do_zeroSup)
				apv->ZeroSuppression();

#ifdef MULTI_THREAD
            __gem_locker.lock();
#endif

			if(do_zeroSup)
				apv->CollectZeroSupHits(event.get_gem_data());
			else
				apv->CollectRawHits(event.get_gem_data());

#ifdef MULTI_THREAD
            __gem_locker.unlock();
#endif
        }
    }
    else 
    {
        std::cout<<__func__<<" waring:: APV "<<addr<<" not found."<<std::endl;
    }
}

// clear all APVs' raw data space
void GEMSystem::Reset()
{
    for(auto &mpd : mpd_slots)
    {
        if(!mpd.second)
            continue;

        mpd.second->APVControl(&GEMAPV::ClearData);
        mpd.second->APVControl(&GEMAPV::ResetPedHist);
    }

    for(auto &det : det_slots)
    {
        if(!det.second)
            continue;
        det.second->Reset();
    }
}

// fill zero suppressed data and re-collect these data in GEM_Strip_Data format
// this is for processing already replayed files
void GEMSystem::FillZeroSupData(const std::vector<GEMZeroSupData> &data_pack,
                                    EventData &event)
{
    // clear all the APVs' hits
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::ResetHitPos);
    }

    // fill in the zero-suppressed data
    for(auto &data : data_pack)
        FillZeroSupData(data);

#ifdef MULTI_THREAD
        __gem_locker.lock();
#endif
    // collect these zero-suppressed hits
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::CollectZeroSupHits, event.get_gem_data());
    }
#ifdef MULTI_THREAD
        __gem_locker.unlock();
#endif
}

// fill zero suppressed data
void GEMSystem::FillZeroSupData(const GEMZeroSupData &data)
{
    GEMAPV *apv = GetAPV(data.addr);

    if(apv != nullptr) {
        apv->FillZeroSupData(data.channel, data.time_sample, data.adc_value);
    }
}

// update EventData to all APVs, this function is for after zero-supperession
// only works for replayed data
void GEMSystem::ChooseEvent(const EventData &data)
{
    // clear all the APVs' hits
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::ClearData);
    }

    for(auto &hit : data.gem_data)
    {
        auto apv = GetAPV(hit.addr.crate, hit.addr.mpd, hit.addr.adc);
        if(apv)
            apv->FillZeroSupData(hit.addr.strip, hit.values);
    }

    for(auto &det : det_slots)
    {
        if(det.second)
            det.second->CollectHits();
    }
}

// reconstruct certain event
void GEMSystem::Reconstruct(const EventData &data)
{
    // only reconstruct physics event
    //if(!data.is_physics_event())
    //    return;

    ChooseEvent(data);
    Reconstruct();
}

void GEMSystem::Reconstruct()
{
    for(auto &det : det_slots)
    {
        if(det.second)
            det.second->Reconstruct(&gem_recon);
    }
}

// fit pedestal for all APVs
// this requires pedestal mode is on, otherwise there won't be any data to fit
void GEMSystem::FitPedestal()
{
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::FitPedestal);
    }
}

// save pedestal file for all APVs
void GEMSystem::SavePedestal(const std::string &name)
const
{
    std::ofstream in_file(name);

    if(!in_file.is_open()) {
        std::cerr << "GEM System: Failed to save pedestal, file "
                  << name << " cannot be opened."
                  << std::endl;
    }

    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::PrintOutPedestal, in_file);
    }
}

// save common mode range for all APVs
void GEMSystem::SaveCommonModeRange(const std::string &name)
const
{
    std::ofstream in_file(name);

    if(!in_file.is_open()) {
        std::cerr << "GEM System: Failed to save common mode range, file "
                  << name << " cannot be opened."
                  << std::endl;
    }

    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::PrintOutCommonModeRange, in_file);
    }
}

// set pedestal mode on/off
// if the pedestal mode is on, filling raw data will also fill the histograms in
// APV for future pedestal fitting
// turn on the pedestal mode will greatly slow down the raw data handling and
// consume a significant amount of memories
void GEMSystem::SetPedestalMode(const bool &m)
{
    PedestalMode = m;
    OnlineMode = !m;
    ReplayMode = !m;

    for(auto &mpd : mpd_slots)
    {
        if(!mpd.second)
            continue;

        if(m)
            mpd.second->APVControl(&GEMAPV::CreatePedHist);
        else
            mpd.second->APVControl(&GEMAPV::ReleasePedHist);
    }
}

// set online mode on/off
// a helper
void GEMSystem::SetOnlineMode(const bool &m)
{
    OnlineMode = m;
    PedestalMode = !m;
    ReplayMode = !m;
}

// set replay mode on/off
// a helper
void GEMSystem::SetReplayMode(const bool &m)
{
    OnlineMode = !m;
    PedestalMode = !m;
    ReplayMode = m;
}

// set trigger time
void GEMSystem::SetTriggerTime(const std::pair<uint32_t, uint32_t> &t)
{
    triggerTime = t;
}

// collect the zero suppressed data from APV
std::vector<GEM_Strip_Data> GEMSystem::GetZeroSupData()
const
{
    std::vector<GEM_Strip_Data> gem_data;

    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::CollectZeroSupHits, gem_data);
    }

    return gem_data;
}

// change the common mode threshold level for all APVs
void GEMSystem::SetUnivCommonModeThresLevel(const float &thres)
{
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::SetCommonModeThresLevel, thres);
    }
}

// change the zero suppression threshold level for all APVs
void GEMSystem::SetUnivZeroSupThresLevel(const float &thres)
{
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::SetZeroSupThresLevel, thres);
    }
}

// change the time sample for all APVs
void GEMSystem::SetUnivTimeSample(const uint32_t &ts)
{
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            mpd.second->APVControl(&GEMAPV::SetTimeSample, ts);
    }
}

// save all APVs' histograms into a root file
void GEMSystem::SaveHistograms(const std::string &path)
const
{
    TFile *f = new TFile(path.c_str(), "recreate");

    for(auto &mpd : mpd_slots)
    {
        if(!mpd.second)
            continue;

        std::string mpd_name = "MPD " + std::to_string(mpd.second->GetID());
        TDirectory *cdtof = f->mkdir(mpd_name.c_str());
        cdtof->cd();

        for(auto apv : mpd.second->GetAPVList())
        {
            std::string adc_name = "ADC " + std::to_string(apv->GetADCChannel());
            TDirectory *cur_dir = cdtof->mkdir(adc_name.c_str());
            cur_dir->cd();

            for(auto hist : apv->GetHistList())
                hist->Write();
        }
    }

    f->Close();
    delete f;
}

// get the whole APV list
std::vector<GEMAPV *> GEMSystem::GetAPVList()
const
{
    std::vector<GEMAPV*> apv_list;
    for(auto &mpd : mpd_slots)
    {
        if(!mpd.second)
            continue;

        std::vector<GEMAPV*> sub_list = mpd.second->GetAPVList();
        apv_list.insert(apv_list.end(), sub_list.begin(), sub_list.end());
    }

    return apv_list;
}

std::vector<GEMMPD*> GEMSystem::GetMPDList()
const
{
    std::vector<GEMMPD*> res;
    for(auto &mpd : mpd_slots)
    {
        if(mpd.second)
            res.push_back(mpd.second);
    }

    return res;
}

std::vector<GEMDetector*> GEMSystem::GetDetectorList()
const
{
    std::vector<GEMDetector*> res;
    for(auto &det : det_slots)
    {
        if(det.second)
            res.push_back(det.second);
    }

    return res;
}


//============================================================================//
// Private Member Functions                                                   //
//============================================================================//

// build gem layers according to the arguments
void GEMSystem::buildLayer(std::list<ConfigValue> &layer_args)
{
    int layer_id, nChamberPerPlane; // layer id, number of chambers in layer
    std::string readout_type; // layer readout type: cartesian/uv
    double x_offset, y_offset; // layer x/y offset
    std::string gem_type; // gem chabmer type in this layer: UVaGEM/INFNGEM
    int nAPVsPerGEMX, nAPVsPerGEMY; // number of APVs on each GEM Chamber X/Y plane
    double x_pitch, y_pitch; // pitch size for x/y plane, usually 0.4 mm(400 microns)
    double x_flip, y_flip; // flip layer direction or not

    layer_args >> layer_id >> nChamberPerPlane >> readout_type >> x_offset 
               >> y_offset >> gem_type >> nAPVsPerGEMX >> nAPVsPerGEMY
               >> x_pitch >> y_pitch >>  x_flip >> y_flip;

    GEMDetectorLayer *layer = new GEMDetectorLayer(layer_id, nChamberPerPlane,
            readout_type, x_offset, y_offset, gem_type, nAPVsPerGEMX,
            nAPVsPerGEMY, x_pitch, y_pitch, x_flip, y_flip);

    if(layer_slots.find(static_cast<uint32_t>(layer_id)) != layer_slots.end()) {
        std::cout<<__func__<<" Warning: found duplicated layer id. Skipped Layer_ID = "
                 << layer_id << std::endl
                 <<" Check your mapping......" << std::endl;
    } else {
        layer -> SetSystem(this);
        layer_slots[static_cast<uint32_t>(layer_id)] = layer;
    }
}

// build the detectors according to the arguments
void GEMSystem::buildDetector(std::list<ConfigValue> &det_args)
{
    // parse apv information
    APV_Entry apv_entry(det_args);

    int det_id = apv_entry.detector_id;
    int layer_id = apv_entry.layer_id;
    int layer_index = apv_entry.gem_pos;

    // add to GEMDetectorLayer
    if(layer_slots.find(static_cast<uint32_t>(layer_id)) == layer_slots.end()) {
        std::cout<<__func__<<" Error: cannot find layer id: "<< layer_id
                 <<" for chamber id: "<<det_id <<std::endl;
        exit(1); // if mapping is wrong, quit the program
    }
    std::string readout = layer_slots[static_cast<uint32_t>(layer_id)] -> GetReadoutType();
    std::string type = layer_slots[static_cast<uint32_t>(layer_id)] -> GetGEMChamberType();
    // detector name = "detector type" + "detector id"
    std::string name = layer_slots[static_cast<uint32_t>(layer_id)] -> GetGEMChamberType() + 
        std::to_string(det_id);

    GEMDetector *new_det = new GEMDetector(readout, type, name, det_id, layer_id, layer_index);
    new_det -> SetGEMLayer(layer_slots[static_cast<uint32_t>(layer_id)]); // set layer

    if(!Register(new_det)) {
        delete new_det;
        return;
    }

    // successfully registered, build maps
    // the map will be needed for add planes into detectors
    det_name_map[name] = new_det;

    // add detectors to layer
    layer_slots[static_cast<uint32_t>(layer_id)] -> AddGEMDetector(new_det); // layer add detector
}

// build the gem chamber planes (for each apv) according to the arguments
// since planes need to be added into existing detectors, it requires all the
// detectors to be built first and a proper detector map is generated
void GEMSystem::buildPlane(std::list<ConfigValue> &pln_args)
{
    // parse apv information
    APV_Entry apv_entry(pln_args);

    // set up plane using layer info
    uint32_t layer_id = static_cast<uint32_t>(apv_entry.layer_id);
    if(layer_slots.find(layer_id) == layer_slots.end()) {
        std::cout<<__func__<<" Error: layer not found: "<<layer_id
                 <<std::endl;
        exit(1);
    }
    GEMDetectorLayer *layer = layer_slots[layer_id];

    std::string det_name = layer -> GetGEMChamberType() + std::to_string(apv_entry.detector_id);
    std::string plane_name = apv_entry.plane_name;
    int type = apv_entry.dimension; // x axis or y axis type = Plane::X or Plane::Y
    int connector = layer -> GetNumberOfAPVsOnChamberPlane(type);
    float pitch = layer -> GetChamberPlanePitch(type);
    float size = (double)connector * APV_STRIP_SIZE * pitch;

    // orient for GEM Plane is reserved for now (not used)
    // orient is only used for GEM APV
    int orient = 1;

    // direct means whether you want to flip direction: x = -x;
    // direct can only be 1 or -1
    // If you install GEM Layers back to back, then one layer can be set to -1
    int direct = layer -> GetFlipByPlaneType(type);

    if(type < 0) // did not find proper type
    {
        std::cout<<"Error: Cannot find Plane Type, X palne or Y plane?"<<std::endl;
        return;
    }

    GEMDetector *det = GetDetector(det_name);
    if(det == nullptr) {// did not find detector
        std::cout << " GEM System Warning: Failed to add plane " << plane_name
                  << " to detector " << det_name
                  << ", please check if the detector exists in GEM configuration file."
                  << std::endl;
        return;
    }

    GEMPlane *new_plane = new GEMPlane(plane_name, type, size, connector, orient, direct);

    // failed to add plane, or plane has already been added
    if(!det->AddPlane(new_plane)) {
        delete new_plane;
        return;
    }
}

// build the MPDs according to the arguments
void GEMSystem::buildMPD(std::list<ConfigValue> &mpd_args)
{
    APV_Entry apv_entry(mpd_args);

    int mpd_id = apv_entry.mpd_id;
    // ip not exist in mpd, reserved for future use
    [[maybe_unused]] std::string mpd_ip = "0.0.0.0";

    int crate_id = apv_entry.crate_id;

    GEMMPD *new_mpd = new GEMMPD(crate_id, mpd_id, mpd_ip);

    // failed to register
    if(!Register(new_mpd)) {
        delete new_mpd;
        return;
    }
}

// build the APVs according to the arguments
// since APVs need to be added into existing MPDs, and be connected to existing
// planes, it requires MPD and planes to be built first and proper maps are generated
void GEMSystem::buildAPV(std::list<ConfigValue> &apv_args)
{
    APV_Entry apv_entry(apv_args);

    std::string plane = apv_entry.plane_name, status = apv_entry.discriptor;
    int crate_id = apv_entry.crate_id;
    int mpd_id = apv_entry.mpd_id, 
        adc_ch = apv_entry.adc_ch, orient = apv_entry.invert, index = apv_entry.apv_pos,
        det_pos = apv_entry.gem_pos;
    uint32_t layer_id = static_cast<uint32_t>(apv_entry.layer_id);
    int det_id = apv_entry.detector_id;

    if(layer_slots.find(layer_id) == layer_slots.end()) {
        std::cout<<__func__<<" Error: layer not found, layer_id: "<<layer_id
                 <<std::endl;
        exit(1);
    }
    GEMDetectorLayer *layer = layer_slots[layer_id];
    std::string det_name = layer -> GetGEMChamberType() + std::to_string(det_id);

    // optional arguments
    unsigned int ts = def_ts;
    float cth = def_cth, zth = def_zth, ctth = def_ctth;

    GEMMPD *mpd = GetMPD(MPDAddress(crate_id, mpd_id));
    if(mpd == nullptr) {// did not find detector
        std::cout << " GEM System Warning: Failed to add APV " << adc_ch
                  << " to MPD " << mpd_id
                  <<" Crate "<<crate_id
                  << ", please check if the MPD exists in GEM configuration file."
                  << std::endl;
        return;
    }

    // vtp pedestal subtraction mode enabled or not? (in the readout list)
    // if enabled, offset will be subtracted online, otherwise not
    bool pedestal_run = (Value<std::string>("VTP Pedestal Subtraction") == "yes" );

    GEMAPV *new_apv = new GEMAPV(orient, det_pos, status, ts, cth, zth, ctth, pedestal_run);
    if(!mpd->AddAPV(new_apv, adc_ch)) { // failed to add APV to MPD
        delete new_apv;
        return;
    }

    // trying to connect to Plane
    GEMDetector *det = GetDetector(det_name);
    if(det == nullptr) {
        std::cout << " GEM System Warning: Cannot find detector " << det_name
                  << ", APV " << mpd_id << ", " << adc_ch
                  << " is not connected to the detector plane. "
                  << std::endl;
        return;
    }

    GEMPlane *pln = det->GetPlane(plane);
    if(pln == nullptr) {
        std::cout << " GEM System Warning: Cannot find plane " << plane
                  << " in detector " << det_name
                  << ", APV " << mpd_id << ", " << adc_ch
                  << " is not connected to the detector plane. "
                  << std::endl;
        return;
    }

    pln->ConnectAPV(new_apv, index);
}

void GEMSystem::PrintStatus()
{
    std::cout<<"gem system has "<<layer_slots.size()<<" layers."<<std::endl;
    for(auto &i: layer_slots)
        i.second -> PrintStatus();
}
