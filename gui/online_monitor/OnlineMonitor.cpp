#include "OnlineMonitor.h"

#include "EventParser.h"
#include "MPDVMERawEventDecoder.h"
#include "MPDSSPRawEventDecoder.h"
#include "SRSRawEventDecoder.h"
#include "RolStruct.h"        // Bank_TagID, Fec_Bank_Tag

#include <et.h>

#include <iostream>

namespace online_monitor {

////////////////////////////////////////////////////////////////////////////////
// ctor: build our own parser + raw decoder (chosen from hardcode.h),
// identical to the offline path (see GEMAnalyzer::Init in
// gui/src/GEMAnalyzer.cpp)

OnlineMonitor::OnlineMonitor()
{
    parser = new EventParser();

    // Pick the raw decoder from hardcode.h, identical to
    // GEMDataHandler::RegisterRawDecoders / GEMAnalyzer::Init.
#ifdef USE_VME
    decoder = new MPDVMERawEventDecoder();
    parser->RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_VME), decoder);
#elif defined(USE_SRS)
    decoder = new SRSRawEventDecoder();
    for(auto &i: Fec_Bank_Tag)
        parser->RegisterRawDecoder(static_cast<int>(i), decoder);
#else
    decoder = new MPDSSPRawEventDecoder();
    parser->RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP), decoder);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// dtor

OnlineMonitor::~OnlineMonitor()
{
    Disconnect();
    delete parser;
    delete decoder;
}

////////////////////////////////////////////////////////////////////////////////
// connect to ET system + create/attach a monitoring station

bool OnlineMonitor::Connect(const std::string &etFile, const std::string &host,
                            int port, const std::string &station)
{
    if(connected)
        Disconnect();

    // --- open the ET system: direct TCP connection to host:port ---
    et_openconfig openconfig;
    et_open_config_init(&openconfig);
    et_open_config_setcast(openconfig, ET_DIRECT);
    et_open_config_sethost(openconfig, host.c_str());
    et_open_config_setserverport(openconfig, port);
    et_open_config_setwait(openconfig, ET_OPEN_WAIT);

    et_sys_id id;
    int status = et_open(&id, etFile.c_str(), openconfig);
    et_open_config_destroy(openconfig);
    if(status != ET_OK) {
        std::cout << "[OnlineMonitor] et_open failed (status " << status << ") for '"
                  << etFile << "' @ " << host << ":" << port << std::endl;
        return false;
    }
    et_id = id;

    // --- create (or join) a non-blocking monitoring station ---
    // Non-blocking so the monitor never back-pressures the DAQ: when the
    // station queue is full new events simply bypass it.
    et_statconfig sconfig;
    et_station_config_init(&sconfig);
    et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
    et_station_config_setcue(sconfig, 100);

    et_stat_id stat;
    status = et_station_create(id, &stat, station.c_str(), sconfig);
    et_station_config_destroy(sconfig);
    if(status != ET_OK && status != ET_ERROR_EXISTS) {
        std::cout << "[OnlineMonitor] et_station_create failed (status " << status
                  << ") for station '" << station << "'" << std::endl;
        et_close(id);
        et_id = nullptr;
        return false;
    }
    et_stat = stat;

    // --- attach to the station ---
    et_att_id att;
    if(et_station_attach(id, stat, &att) != ET_OK) {
        std::cout << "[OnlineMonitor] et_station_attach failed for station '"
                  << station << "'" << std::endl;
        et_close(id);
        et_id = nullptr;
        et_stat = -1;
        return false;
    }
    et_att = att;

    connected = true;
    std::cout << "[OnlineMonitor] connected to ET '" << etFile << "' @ " << host
              << ":" << port << ", station '" << station << "'" << std::endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// pull + decode one event (non-blocking)

bool OnlineMonitor::NextEvent()
{
    if(!connected)
        return false;

    et_event *pe = nullptr;
    // ET_ASYNC: return immediately if nothing is available -- keeps the GUI
    // thread responsive when driven from a QTimer.
    int status = et_event_get(et_id, et_att, &pe, ET_ASYNC, nullptr);
    if(status != ET_OK) {
        if(status == ET_ERROR_EMPTY || status == ET_ERROR_TIMEOUT)
            return false;                       // simply no event this tick
        std::cout << "[OnlineMonitor] et_event_get error (status " << status
                  << "); marking disconnected" << std::endl;
        connected = false;                      // dead/closed system etc.
        return false;
    }

    void  *data = nullptr;
    size_t len  = 0;                            // length in BYTES
    et_event_getdata(pe, &data);
    et_event_getlength(pe, &len);

    // An ET event payload is one EVIO event. ParseEvent wants a uint32_t word
    // count, so convert bytes -> words. After this the decoder's GetAPV()/
    // GetAPVDataFlags() hold the decoded event.
    if(data != nullptr && len >= sizeof(uint32_t)) {
        parser->ParseEvent(static_cast<const uint32_t*>(data),
                           static_cast<uint32_t>(len / sizeof(uint32_t)));
    }

    // return the event to the system so it can be recycled
    et_event_put(et_id, et_att, pe);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// detach + close

void OnlineMonitor::Disconnect()
{
    if(et_id != nullptr) {
        if(et_att >= 0)
            et_station_detach(et_id, et_att);
        et_close(et_id);
    }
    et_id   = nullptr;
    et_att  = -1;
    et_stat = -1;
    connected = false;
}

////////////////////////////////////////////////////////////////////////////////
// decoded data accessors (mirror GEMAnalyzer)

const std::unordered_map<APVAddress, std::vector<int>> & OnlineMonitor::GetData() const
{
    return decoder ? decoder->GetAPV() : empty_data;
}

const std::unordered_map<APVAddress, APVDataType> & OnlineMonitor::GetDataFlags() const
{
    return decoder ? decoder->GetAPVDataFlags() : empty_flags;
}

} // namespace online_monitor
