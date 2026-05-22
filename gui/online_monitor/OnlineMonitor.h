#ifndef ONLINE_MONITOR_H
#define ONLINE_MONITOR_H

////////////////////////////////////////////////////////////////////////////////
// OnlineMonitor
//
// Encapsulates ALL CODA ET (Event Transfer) client code so the rest of the
// GUI never needs to know about ET. It attaches to a live ET system, pulls
// one EVIO event at a time, decodes it with the same EventParser +
// MPDSSPRawEventDecoder used by the offline path, and exposes the decoded
// per-APV maps through GetData()/GetDataFlags() -- the exact same interface
// as GEMAnalyzer, so Viewer can treat the online and offline feeds
// identically.
//
// IMPORTANT: this header deliberately exposes NO ET types. The ET system
// handle is held as void* (et_sys_id is itself typedef'd to void*) and the
// attachment/station ids as int. <et.h> is included only in the .cpp. That
// way the gui application links libonline_monitor without needing the ET
// include path at all.
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <unordered_map>

#include "MPDDataStruct.h"   // APVAddress, APVDataType
#include "hardcode.h"        // USE_VME / USE_SRS / (else) SSP decoder switch

class EventParser;
// The raw decoder is chosen at compile time from hardcode.h, mirroring
// GEMAnalyzer / GEMDataHandler.
#ifdef USE_VME
class MPDVMERawEventDecoder;
#elif defined(USE_SRS)
class SRSRawEventDecoder;
#else
class MPDSSPRawEventDecoder;
#endif

namespace online_monitor {

class OnlineMonitor
{
public:
    OnlineMonitor();
    ~OnlineMonitor();

    // Attach to an ET system via a direct host:port connection, then create
    // (or join, if it already exists) the named station and attach to it.
    //   etFile  : the ET system file name (e.g. /tmp/et_sys_myexp)
    //   host    : host running the ET system
    //   port    : ET system TCP server port
    //   station : name of the monitoring station to create/attach
    // Returns true on success.
    bool Connect(const std::string &etFile, const std::string &host,
                 int port, const std::string &station);

    bool IsConnected() const { return connected; }

    // Pull and decode ONE event (non-blocking). Returns true if an event was
    // received and decoded (GetData/GetDataFlags then hold its contents),
    // false if no event was available or on error.
    bool NextEvent();

    void Disconnect();

    // Decoded per-APV data for the most recently received event. Same return
    // types as GEMAnalyzer::GetData()/GetDataFlags().
    const std::unordered_map<APVAddress, std::vector<int>> & GetData() const;
    const std::unordered_map<APVAddress, APVDataType> & GetDataFlags() const;

private:
    // ET handles kept ET-type-free (see header note above).
    void *et_id  = nullptr;   // et_sys_id  (typedef void*)
    int   et_att = -1;        // et_att_id
    int   et_stat = -1;       // et_stat_id
    bool  connected = false;

    EventParser           *parser  = nullptr;
#ifdef USE_VME
    MPDVMERawEventDecoder *decoder = nullptr;
#elif defined(USE_SRS)
    SRSRawEventDecoder    *decoder = nullptr;
#else
    MPDSSPRawEventDecoder *decoder = nullptr;
#endif

    // safe fallbacks returned before any event / when decoder is unavailable
    std::unordered_map<APVAddress, std::vector<int>> empty_data;
    std::unordered_map<APVAddress, APVDataType>      empty_flags;
};

} // namespace online_monitor

#endif
