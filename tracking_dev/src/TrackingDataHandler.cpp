#include "TrackingDataHandler.h"
#include "AbstractDetector.h"
#include "Tracking.h"
#include "GEMPlane.h"

namespace tracking_dev
{
    TrackingDataHandler::TrackingDataHandler()
    {
        //Init();
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

        if(gem_sys == nullptr)
	{
            gem_sys = new GEMSystem();
            gem_sys -> Configure("config/gem.conf");
        } else {
	    std::cout<<"INFO::: gem_sys is external in TrackingDataHandler Init()"<<std::endl;
	}

        if(data_handler == nullptr)
	{
            data_handler = new GEMDataHandler();
            data_handler -> SetGEMSystem(gem_sys);
        } else {
	    std::cout<<"INFO::: gem_data_handler is external in TrackingDataHandler Init()"<<std::endl;
	}

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
	    if(fDet[i]!=nullptr)
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
        data_handler -> DecodeEvent(event_counter);
        event_counter++;

        PackageEventData();
    }

    void TrackingDataHandler::PackageEventData()
    {
        unsigned int N = detector_list.size();
        for(unsigned int i=0; i<N; i++)
        {
            int layer_id_ = detector_list[i] -> GetLayerID();
            TransferDetector(detector_list[i], fDet[layer_id_]);
        }
    }

    void TrackingDataHandler::TransferDetector(GEMDetector* gem_det, AbstractDetector *det)
    {
        const std::vector<GEMHit> & detector_2d_hits = gem_det -> GetHits();
        int layer = gem_det -> GetLayerID();

	// if this gem is not part of the tracking system, then skip its data
	if(!gem_cuts -> is_tracking_layer(layer))
	    return;

        double z_det = det -> GetZPosition();
        det -> Reset();

        for(auto &i: detector_2d_hits) {
            point_t p(i.x, i.y, z_det, i.x_charge, i.y_charge, i.x_peak, i.y_peak, 
                    i.x_max_timebin, i.y_max_timebin, i.x_size, i.y_size);

            // currently use layer id as module id, this works for now since one layer
            // has only one module; for the future, module_id should be read
            // from mapping file, which is easy to implement, since we already have
            // gem_det pointer here: module_id = gem_det -> GetModuleID();
            p.module_id = layer;
            p.layer_id = layer;

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
