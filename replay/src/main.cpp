#include <iostream>
#include <chrono>
#include "ConfigArgs.h"
#include "EvioFileReader.h"
#include "GEMSystem.h"
#include "GEMDataHandler.h"
#include "GEMRootClusterTree.h"
#include "TrackingDataHandler.h"
#include "Tracking.h"
#include "TrackingUtility.h"
#include "AbstractDetector.h"

#define GENERATE_GEM_HISTOS_CXX
#include "generate_gem_histos.h"

#define PROGRESS_COUNT 1000

void fill_tracking_result(tracking_dev::TrackingDataHandler *tracking_data_handler,
	tracking_dev::Tracking *tracking, GEMRootClusterTree *gem_tree);

int main(int argc, char* argv[])
{
    ConfigArgs arg_parser;
    arg_parser.AddHelps({"-h", "--help"});
    arg_parser.AddPositional("raw_data", "raw data in evio format");
    arg_parser.AddArgs<std::string>({"--output_root_path"}, "output_root_path", "output data file in root format", "");
    arg_parser.AddArg<int>("-n", "nev", "number of events to process (< 0 means all)", -1);
    arg_parser.AddArg<int>("-s", "start_event", "start analysis from event number (default to 0)", 0);
    arg_parser.AddArg<bool>("-c", "c_evio_to_root", "convert evio to root files (no zero sup)", false);
    arg_parser.AddArg<bool>("-t", "replay_hit", "replay hit root tree (with zero sup)", false);
    arg_parser.AddArg<bool>("-z", "replay_cluster", "replay cluster root tree (with clustering)", true);
    arg_parser.AddArgs<std::string>({"--pedestal"}, "pedestal_file", "pedestal file", 
	    "database/gem_ped_55.dat");
    arg_parser.AddArgs<std::string>({"--common_mode"}, "common_mode_file", "common mode file",
	    "database/CommonModeRange_55.txt");
    arg_parser.AddArgs<std::string>({"--tracking"}, "tracking_switch", " switch on/off tracking",
	    "off");

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

    // -: gem data handler
    GEMDataHandler *gem_data_handler = new GEMDataHandler();
    gem_data_handler -> SetGEMSystem(gem_system);
    gem_data_handler -> SetEvioFileReader(evio_reader);
    gem_data_handler -> RegisterRawDecoders();

    // -: configure replay
    unsigned short mode = (((unsigned short)args["replay_cluster"].Bool() & 0x1 ) << 2) |
	(((unsigned short)args["c_evio_to_root"].Bool() & 0x1) << 1) |
	((unsigned short)args["replay_hit"].Bool() & 0x1);
    if((mode != 4) && (mode != 2) && (mode != 1)) {
	std::cout<<"ERROR:: only one mode is allowed. Please set paramter correctly"<<std::endl;
	std::cout<<"        choose one of the following three modes (1 = turn on; 0 = turn off)"<<std::endl;
	std::cout<<"        replay hit mode: [-t 1]; replay cluster mode: [-z 1]; pure evio to file conversion mode: [-c 1]"
	    <<std::endl;
	exit(0);
    }

    // for hit replay
    if(args["replay_hit"].Bool() || args["c_evio_to_root"].Bool())
    {
	gem_data_handler -> TurnOffClustering();

	// turn off zero suppression if just want to do a evio to root convert
	// this will turn off zero suppression
	if(args["c_evio_to_root"].Bool())
	    gem_data_handler -> TurnOnbEvio2RootFiles();
    }
    // for cluster replay
    if(args["replay_cluster"].Bool()) {
	gem_data_handler -> TurnOnClustering();
    }
    if(args["output_root_path"].String().size() > 0)
	gem_data_handler -> SetOutputPath(args["output_root_path"].String().c_str());
    gem_data_handler -> SetupReplay(args["raw_data"].String(), 0, -1, args["pedestal_file"].String(), args["common_mode_file"].String());

    // -: tracking
    tracking_dev::TrackingDataHandler *tracking_data_handler = new tracking_dev::TrackingDataHandler();
    tracking_data_handler -> SetGEMSystem(gem_system);
    tracking_data_handler -> SetGEMDataHandler(gem_data_handler);
    tracking_data_handler -> Init();
    tracking_data_handler -> SetupDetector();
    tracking_dev::Tracking *new_tracking = tracking_data_handler -> GetTrackingHandle();
    bool is_tracking_on = args["tracking_switch"].String() == "on";
    if(is_tracking_on) {
	std::cout<<"INFO:::: Tracking is turned on."<<std::endl;
    }
    else {
	std::cout<<"INFO:::: Tracking is turned off."<<std::endl;
    }

    // -: open evio file
    evio_reader -> SetFile(args["raw_data"].String());
    if(! evio_reader -> OpenFile() )
    {
	std::cout<<"Cannot open evio file: "<<args["raw_data"].String()<<std::endl;
	std::cout<<"please check your evio file path."<<std::endl;
	return 0;
    }

    // -: data quality check histograms
    quality_check_histos::pass_handles(gem_system, tracking_data_handler);
    quality_check_histos::set_output_name(gem_data_handler -> GetClusterTreeOutputFileName());

    // -: do replay
    auto time_1 = std::chrono::steady_clock::now();
    auto time_2 = std::chrono::steady_clock::now();

    int start_event = args["start_event"].Int();
    int max_event = args["nev"].Int();
    int event_counter = 0;
    const uint32_t *pBuf;
    uint32_t fBufLen;
    while(evio_reader -> ReadNoCopy(&pBuf, &fBufLen) == S_SUCCESS)
    {
	if(event_counter < start_event) {
	    event_counter++;
	    continue;
	}

	if((event_counter % PROGRESS_COUNT) == 0) {
	    time_2 = std::chrono::steady_clock::now();
	    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_2 - time_1).count();
	    std::cout << "Processed events - " << event_counter << " - " 
		<< time_elapsed <<" milliseconds per " << PROGRESS_COUNT << " events." << "\r" << std::flush;
	    time_1 = time_2;
	}

	// raw cluster/hit process
	gem_data_handler -> ProcessEvent(pBuf, fBufLen, event_counter);

	if(is_tracking_on) {
	    // tracking only works on clustering mode
	    if(args["replay_cluster"].Bool()) {
		// tracking process
		tracking_data_handler -> ClearPrevEvent();
		tracking_data_handler -> PackageEventData();
		new_tracking -> FindTracks();

		fill_tracking_result(tracking_data_handler, new_tracking, gem_data_handler -> GetClusterTree());
	    }
	}

	gem_data_handler -> EndofThisEvent(event_counter);

	event_counter++;
	// fill data quality check histos
	quality_check_histos::fill_gem_histos(event_counter - start_event);

	if(max_event > 0 && event_counter > max_event)
	    break;
    }

    std::cout<<"total event: "<<event_counter<<std::endl;
    gem_data_handler -> Write();
    quality_check_histos::generate_tracking_based_2d_efficiency_plots();
    quality_check_histos::save_histos();

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
    gem_tree->fNtracks_found = tracking -> GetNTracksFound();
    gem_tree->fNAllGoodTrackCandidates = tracking -> GetNGoodTrackCandidates();
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
