#ifndef TRACKING_DATA_HANDLER_H
#define TRACKING_DATA_HANDLER_H

#include "GEMSystem.h"
#include "GEMDetector.h"
#include "GEMDataHandler.h"
#include "ConfigObject.h"
#include "CoordSystem.h"
#include "Cuts.h"

namespace tracking_dev {

    class Tracking;
    class AbstractDetector;

    class TrackingDataHandler
    {
    public:
        TrackingDataHandler();
        ~TrackingDataHandler();

        void Init();
        void SetupDetector();
        void Configure();
        void SetOnlineMode(bool b);
        void SetReplayMode(bool b);
        void NextEvent();
        void ClearPrevEvent();
        void PackageEventData();
        void TransferDetector(GEMDetector *, AbstractDetector*);

        // setters
        void SetGEMSystem(GEMSystem *s) {gem_sys = s;}
        void SetGEMDataHandler(GEMDataHandler *h){data_handler = h;}
        void SetCoordSystem(CoordSystem *c){coord_system = c;}
        void SetTrackingHandler(Tracking *t){tracking = t;}
        void SetEvioFile(const char* p);

        // getters
        void GetCurrentEvent();
        unsigned int GetNumberofDetectors(){return detector_list.size();}
        AbstractDetector* GetDetector(int i){return fDet[i];}
        bool IsOnlineMode(){return is_online_mode;}
        GEMSystem * GetGEMSystem(){return gem_sys;}
        CoordSystem *GetCoordSystem(){return coord_system;}
        Tracking *GetTrackingHandle(){return tracking;}

    private:
        GEMDataHandler *data_handler = nullptr;
        GEMSystem *gem_sys = nullptr;

        //std::string input_file = "../data/hallc_fadc_ssp_4680.evio.0";
        std::string input_file = "../data/hallc_fadc_ssp_4818.evio.1";
        //std::string input_file = "../data/hallc_fadc_ssp_4762.evio.1";
        std::string pedestal_file;
        std::string common_mode_file;

        ConfigObject txt_parser;
        Cuts *gem_cuts;

        bool is_configured = false;
        bool is_online_mode = true;

        // 
        Tracking *tracking;
        std::vector<AbstractDetector*> fDet;
        std::vector<GEMDetector*> detector_list;

        //
        CoordSystem *coord_system;
        
        //
        int event_counter = 0;
    };
};

#endif
