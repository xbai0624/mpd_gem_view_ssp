#include <iostream>
#include "ConfigArgs.h"
#include "EvioFileReader.h"
#include "GEMSystem.h"
#include "GEMDataHandler.h"
#include "GEMRootClusterTree.h"
#include "TrackingDataHandler.h"
#include "Tracking.h"
#include "TrackingUtility.h"
#include "AbstractDetector.h"

void fill_tracking_result(tracking_dev::TrackingDataHandler *tracking_data_handler,
        tracking_dev::Tracking *tracking, GEMRootClusterTree *gem_tree);

int main(int argc, char* argv[])
{
    ConfigArgs arg_parser;
    arg_parser.AddHelps({"-h", "--help"});
    arg_parser.AddPositional("raw_data", "raw data in evio format");
    arg_parser.AddArgs<std::string>({"--ouput_root_path"}, "output_root_path", "output data file in root format", "");
    arg_parser.AddArg<int>("-n", "nev", "number of events to process (< 0 means all)", -1);
    arg_parser.AddArg<int>("-s", "start_event", "start analysis from event number (default to 0)", 0);
    arg_parser.AddArgs<std::string>({"--pedestal"}, "pedestal_file", "pedestal file", 
            "database/gem_ped_55.dat");
    arg_parser.AddArgs<std::string>({"--common_mode"}, "common_mode_file", "common mode file",
            "database/CommonModeRange_55.txt");

    auto args = arg_parser.ParseArgs(argc, argv);

    // show arguments
    for(auto &it : args) {
        std::cout << it.first << ": " << it.second.String() << std::endl;
    }

    // -: evio file reader
    EvioFileReader *evio_reader = new EvioFileReader();

    // -: gem system
    GEMSystem *gem_system = new GEMSystem();
    gem_system -> Configure("config/gem.conf");
    gem_system -> ReadPedestalFile();

    // -: gem data handler
    GEMDataHandler *gem_data_handler = new GEMDataHandler();
    gem_data_handler -> SetGEMSystem(gem_system);
    gem_data_handler -> SetEvioFileReader(evio_reader);
    gem_data_handler -> RegisterRawDecoders();
    // for hit replay
    //gem_data_handler -> TurnOffClustering();
    // for cluster replay
    gem_data_handler -> TurnOnClustering();
    gem_data_handler -> SetupReplay(args["raw_data"].String(), 0, -1, args["pedestal_file"].String(), args["common_mode_file"].String());

    // -: tracking
    tracking_dev::TrackingDataHandler *tracking_data_handler = new tracking_dev::TrackingDataHandler();
    tracking_data_handler -> SetGEMSystem(gem_system);
    tracking_data_handler -> SetupDetector();
    tracking_dev::Tracking *new_tracking = tracking_data_handler -> GetTrackingHandle();

    // -: open evio file
    evio_reader -> SetFile(args["raw_data"].String());
    if(! evio_reader -> OpenFile() )
    {
        std::cout<<"Cannot open evio file: "<<args["raw_data"].String()<<std::endl;
        std::cout<<"please check your evio file path."<<std::endl;
        return 0;
    }

    // -: do replay
    int event_counter = 0;
    const uint32_t *pBuf;
    uint32_t fBufLen;
    while(evio_reader -> ReadNoCopy(&pBuf, &fBufLen) == S_SUCCESS)
    {
        // raw cluster/hit process
        gem_data_handler -> ProcessEvent(pBuf, fBufLen, event_counter);

        // tracking process
        tracking_data_handler -> ClearPrevEvent();
        tracking_data_handler -> PackageEventData();
        new_tracking -> FindTracks();

        fill_tracking_result(tracking_data_handler, new_tracking, gem_data_handler -> GetClusterTree());

        gem_data_handler -> EndofThisEvent(event_counter);
        event_counter++;
    }
    std::cout<<"total event: "<<event_counter<<std::endl;
    gem_data_handler -> Write();

    return 0;
}

void fill_tracking_result(tracking_dev::TrackingDataHandler *tracking_data_handler,
        tracking_dev::Tracking *tracking, GEMRootClusterTree *gem_tree)
{
    if(gem_tree == nullptr) {
        // gem_tree is dynamiclly created, this is possible
        return;
    }

    if(tracking_data_handler == nullptr || tracking == nullptr) {
        std::cout<<"Error: tracking handlers are nullptr."<<std::endl;
        exit(0);
    }

    gem_tree -> ClearPrevTracks();

    double xt, yt, xp, yp, chi2ndf;
    bool found_track = tracking -> GetBestTrack(xt, yt, xp, yp, chi2ndf);
    if(!found_track) return;

    gem_tree->besttrack = tracking -> GetBestTrackIndex();
    gem_tree->fNtracks_found = tracking -> GetNGoodTrackCandidates();
    gem_tree->fNhitsOnTrack = tracking -> GetAllTrackNhits();
    gem_tree->fXtrack = tracking -> GetAllXtrack();
    gem_tree->fYtrack = tracking -> GetAllYtrack();
    gem_tree->fXptrack = tracking -> GetAllXptrack();
    gem_tree->fYptrack = tracking -> GetAllYptrack();
    gem_tree->fChi2Track = tracking -> GetAllChi2ndf();

    gem_tree->ngoodhits = tracking -> GetTotalNgoodHits();
    gem_tree->fHitXlocal = tracking -> GetAllXlocal();
    gem_tree->fHitYlocal = tracking -> GetAllYlocal();
    gem_tree->fHitZlocal = tracking -> GetAllZlocal();
    gem_tree->hit_track_index = tracking -> GetAllHitTrackIndex();
    gem_tree->fHitModule = tracking -> GetAllHitModule();

    // below is for best track - the one with minimum chi2
    // for now, we only save layer index for best track
    gem_tree->fBestTrackHitLayer = tracking -> GetBestTrackLayerIndex();
    tracking_dev::point_t pt(xt, yt, 0);
    tracking_dev::point_t dir(xp, yp, 1.);
    const std::vector<int> &hit_index = tracking -> GetBestTrackHitIndex();

    for(unsigned int i=0; i<gem_tree->fBestTrackHitLayer.size(); i++)
    {
        int layer = gem_tree->fBestTrackHitLayer[i];
        int hit_id = hit_index[i];

        tracking_dev::point_t p_local = (tracking_data_handler -> GetDetector(layer)) -> Get2DHit(hit_id);
        double z = (tracking_data_handler -> GetDetector(layer)) -> GetZPosition();
        tracking_dev::point_t p_projected = (tracking -> GetTrackingUtility()) -> projected_point(pt, dir, z);

        (gem_tree->fBestTrackHitXprojected).push_back(p_projected.x);
        (gem_tree->fBestTrackHitYprojected).push_back(p_projected.y);
        (gem_tree->fBestTrackHitResidU).push_back(p_projected.x - p_local.x);
        (gem_tree->fBestTrackHitResidV).push_back(p_projected.y - p_local.y);
        (gem_tree->fBestTrackHitUADC).push_back(p_local.x_peak);
        (gem_tree->fBestTrackHitVADC).push_back(p_local.y_peak);
        (gem_tree->fBestTrackHitIsampMaxUstrip).push_back(p_local.x_max_timebin);
        (gem_tree->fBestTrackHitIsampMaxVstrip).push_back(p_local.y_max_timebin);
    }
}
