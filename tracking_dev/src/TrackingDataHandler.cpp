#include "TrackingDataHandler.h"
#include "VirtualDetector.h"
#include "Tracking.h"
#include "GEMPlane.h"
#include "Cuts.h"

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
        for(auto &i: fDet)
        {
            if(i.second!=nullptr)
                i.second -> Reset();
        }

        for(auto &i: fLayer)
        {
            if(i.second != nullptr)
                i.second -> Reset();
        }
    }

    void TrackingDataHandler::SetupDetector()
    {
        detector_list = gem_sys -> GetDetectorList();
        std::unordered_map<int, bool> layer_id_set;

        // detector
        for(auto &det: detector_list)
        {
            int mod_id = det -> GetDetID(); // detector module id
            int layer_id = det -> GetLayerID();

            point_t dimension = coord_system -> GetDetectorDimension(mod_id);
            point_t origin = coord_system -> GetDetectorPosition(mod_id);

            fDet[mod_id] = new VirtualDetector();
            fDet[mod_id] -> SetOrigin(origin);

            fDet[mod_id] -> SetDimension(dimension);
            fDet[mod_id] -> SetDetModuleID(mod_id);
            fDet[mod_id] -> SetLayerID(layer_id);

            bool is_tracker = coord_system -> IsInTrackerSystem(mod_id);
            // if there is at least one detector in this layer is participating in tracking
            // then the entire layer is considered for tracking, we don't support only 
            // using part of the layer for tracking
            if(layer_id_set.find(layer_id) == layer_id_set.end())
                layer_id_set[layer_id] = false;
            if(is_tracker) {
                layer_id_set[layer_id] = true;
            }

            vDetModuleIDs.push_back(mod_id);
        }

        // layer
        auto deduct_layer_dimension = [&](int layer_id) -> point_t
        {
            double x_l = 0, x_r = 0, y_b = 0, y_t = 0;
            double z = -99999.;
            for(auto &i: fDet) {
                auto & det = i.second;
                if(det -> GetLayerID() != layer_id)
                    continue;

                auto origin = det -> GetOrigin();
                auto dim = det -> GetDimension();

                if(origin.x - dim.x/2 < x_l)
                    x_l = origin.x - dim.x/2;
                if(origin.x + dim.x/2 > x_r)
                    x_r = origin.x + dim.x/2;
                if(origin.y - dim.y/2 < y_b)
                    y_b = origin.y - dim.y/2;
                if(origin.y + dim.y/2 > y_t)
                    y_t = origin.y + dim.y/2;
                if( z == -99999.)
                    z = origin.z;
                else if(origin.z != z) {
                    std::cout<<"ERROR: detectors in same layer must have same z. check you tracking config file."
                        <<std::endl;
                    exit(0);
                }
            }

            double x_d = x_r - x_l;
            double y_d = y_t - y_b;
            return point_t(x_d, y_d, z);
        };

        tracking = new Tracking();
        for(auto &it: layer_id_set)
        {
            int i = it.first;
            auto dim = deduct_layer_dimension(i);
            fLayer[i] = new VirtualDetector();
            fLayer[i] -> SetOrigin(point_t(0, 0, dim.z));
            fLayer[i] -> SetDimension(dim);
            fLayer[i] -> SetLayerID(i);
            // for layer virtual detectors, it does not have a physical detector module ID
            vLayerIDs.push_back(i);

            auto v = Cuts::Instance().__get("grid width").arr<double>();
            double s = Cuts::Instance().__get("grid shift").val<double>();
            fLayer[i] -> SetGridWidth(v[0], v[1]);
            fLayer[i] -> SetGridShift(s);

            if(it.second)
                tracking -> AddLayer(i, fLayer[i]);
        }

        tracking -> CompleteSetup();
    }

    VirtualDetector* TrackingDataHandler::GetDetector(int i) const
    {
        // find detector by detector module_id
        auto it = fDet.find(i);

        if(it == fDet.end()) return nullptr;
        return it->second;
    }

    VirtualDetector* TrackingDataHandler::GetLayer(int i) const
    {
        // find layer by layer_id
        auto it = fLayer.find(i);

        if(it == fLayer.end()) return nullptr;
        return it->second;
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
            int module_id_ = detector_list[i] -> GetDetID();
            TransferDetector(detector_list[i], fDet[module_id_]);
        }

        CombineDetHitsToLayer();
    }

    void TrackingDataHandler::TransferDetector(GEMDetector* gem_det, VirtualDetector *det)
    {
        const std::vector<GEMHit> & detector_2d_hits = gem_det -> GetHits();
        int module_id = gem_det -> GetDetID();
        int layer_id = gem_det -> GetLayerID();

        // if this gem is not part of the tracking system, then skip its data
        // xinzhan: instead of skip its data, if this gem is not part of the tracking system
        //          then it won't be passed to Tracking handle, so it is fine to add back the
        //          data in here (we need its data for tracker based residue distribution study)
        //if(!Cuts::Instance().is_tracking_detector(layer))
        //    return;

        double z_det = det -> GetZPosition();
        det -> Reset();

        for(auto &i: detector_2d_hits) {
            point_t p(i.x, i.y, z_det, i.x_charge, i.y_charge, i.x_peak, i.y_peak, 
                    i.x_max_timebin, i.y_max_timebin, i.x_size, i.y_size);

            p.module_id = module_id;
            p.layer_id = layer_id;

            coord_system -> Transform(p, module_id);
            det -> AddHit(p);
        }
    }

    void TrackingDataHandler::CombineDetHitsToLayer()
    {
        for(auto &i: fDet) {
            int layer_id = i.second -> GetLayerID();
            const std::vector<point_t>& hits = i.second -> GetGlobalHits();

            auto it = fLayer.find(layer_id);
            if(it == fLayer.end() || !it->second)
                continue;

            for(const point_t &j: hits)
            {
                it->second -> AddHit(j);
            }
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
