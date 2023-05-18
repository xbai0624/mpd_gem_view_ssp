#include "TrackingDataHandler.h"
#include "AbstractDetector.h"
#include "Tracking.h"
#include "GEMPlane.h"

namespace tracking_dev
{
    TrackingDataHandler::TrackingDataHandler()
    {
        Init();
    }

    TrackingDataHandler::~TrackingDataHandler()
    {
    }

    void TrackingDataHandler::Init()
    {
        if(!txt_parser.ReadConfigFile("config/gem.conf")) {
            std::cout<<__func__<<" failed loading config file."<<std::endl;
            exit(1);
        }

        data_handler = new GEMDataHandler();
        gem_sys = new GEMSystem();
        gem_sys -> Configure("config/gem.conf");

        data_handler -> SetGEMSystem(gem_sys);

        pedestal_file = txt_parser.Value<std::string>("GEM Pedestal");
        common_mode_file = txt_parser.Value<std::string>("GEM Common Mode");

        coord_system = new CoordSystem();

        gem_cuts = new Cuts();
    }

    void TrackingDataHandler::SetEvioFile(const char* p)
    {
        input_file = p;

        if(!is_configured)
            Configure();
        else
            data_handler -> OpenEvioFile(input_file);
    }

    void TrackingDataHandler::ClearPrevEvent()
    {
        unsigned int nDET = detector_list.size();
        for(unsigned int i=0; i<nDET; i++)
        {
            fDet[i] -> Reset();
        }
    }

    void TrackingDataHandler::SetupDetector()
    {
        tracking = new Tracking();
        detector_list = gem_sys -> GetDetectorList();

        unsigned int nDET = detector_list.size();
        fDet.resize(nDET, nullptr);

        for(auto &det: detector_list)
        {
            int i = det -> GetLayerID();

            point_t dimension = coord_system -> GetLayerDimension(i);
            point_t origin = coord_system -> GetLayerPosition(i);

            fDet[i] = new AbstractDetector();
            fDet[i] -> SetOrigin(origin);

            auto v = gem_cuts -> __get("grid width").arr<double>();
            double s = gem_cuts -> __get("grid shift").val<double>();
            fDet[i] -> SetGridWidth(v[0], v[1]);
            fDet[i] -> SetGridShift(s);
 
            fDet[i] -> SetDimension(dimension);

            tracking -> AddDetector(i, fDet[i]);
        }

        tracking -> CompleteSetup();
    }

    void TrackingDataHandler::Configure()
    {
        // we don't want any tree output from GEMDataHandler
        data_handler -> DisableOutputRootTree();

        data_handler -> SetOnlineMode(true);

        gem_sys -> ReadPedestalFile(pedestal_file, common_mode_file);

        data_handler -> OpenEvioFile(input_file);
        data_handler -> RegisterRawDecoders();
        data_handler -> TurnOnClustering();

        is_configured = true;
    }

    void TrackingDataHandler::SetOnlineMode(bool m)
    {
        data_handler -> SetOnlineMode(m);

        is_online_mode = m;
    }

    void TrackingDataHandler::SetReplayMode(bool m)
    {
        data_handler -> SetReplayMode(m);
    }

    void TrackingDataHandler::NextEvent()
    {
        static int count = 0;
        data_handler -> DecodeEvent(count);

        PackageEventData();
    }

    void TrackingDataHandler::PackageEventData()
    {
        unsigned int N = detector_list.size();
        for(unsigned int i=0; i<N; i++)
        {
            TransferDetector(detector_list[i], fDet[i]);
        }
    }

    void TrackingDataHandler::TransferDetector(GEMDetector* gem_det, AbstractDetector *det)
    {
        const std::vector<GEMHit> & detector_2d_hits = gem_det -> GetHits();
        int layer = gem_det -> GetLayerID();
        double z_det = det -> GetZPosition();

        det -> Reset();
        for(auto &i: detector_2d_hits) {
            point_t p(i.x, i.y, z_det, i.x_charge, i.y_charge, i.x_peak, i.y_peak, 
                    i.x_max_timebin, i.y_max_timebin, i.x_size, i.y_size);

            coord_system -> Transform(p, layer);
            det -> AddHit(p);
        }
    }

    void TrackingDataHandler::GetCurrentEvent()
    {
        if(!is_configured) {
            Configure();
        }

        ClearPrevEvent();

        // process next event
        NextEvent();
    }
};
