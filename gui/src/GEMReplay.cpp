#include "GEMReplay.h"
#include "APVStripMapping.h"

////////////////////////////////////////////////////////////////////////////////
// ctor

GEMReplay::GEMReplay()
{
    if(!txt_parser.ReadConfigFile("config/gem.conf")) {
        std::cout<<__func__<<" failed loading config file."<<std::endl;
        exit(1);
    }

    apv_strip_mapping::Mapping::Instance() -> LoadMap(txt_parser.Value<std::string>("GEM Map").c_str());

    data_handler = new GEMDataHandler();
    gem_sys = new GEMSystem();
    gem_sys -> Configure("config/gem.conf");

    data_handler -> SetGEMSystem(gem_sys);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

GEMReplay::~GEMReplay()
{
    // place holder
}


////////////////////////////////////////////////////////////////////////////////
// generate pedestal

void GEMReplay::GeneratePedestal()
{
    gem_sys -> SetPedestalMode(true);
    data_handler -> SetMaxPedestalEvents(max_pedestal_events);
    data_handler -> Replay(ifile, split_min, split_max,
            pedestal_input_file, common_mode_input_file,
            pedestal_output_file, commonMode_output_file);
}


////////////////////////////////////////////////////////////////////////////////
// replay to root file (extract hit)

void GEMReplay::ReplayHit()
{
    gem_sys -> SetReplayMode(true);

    // turn off clustering
    data_handler -> TurnOffClustering();

    data_handler -> Replay(ifile, split_min, split_max,
            pedestal_input_file, common_mode_input_file,
            pedestal_output_file, commonMode_output_file);
}

////////////////////////////////////////////////////////////////////////////////
// replay to root file (extract cluster)

void GEMReplay::ReplayCluster()
{
    gem_sys -> SetReplayMode(true);

    // turn on clustering
    data_handler -> TurnOnClustering();

    data_handler -> Replay(ifile, split_min, split_max,
            pedestal_input_file, common_mode_input_file,
            pedestal_output_file, commonMode_output_file);

    // turn off clustering
    data_handler -> TurnOffClustering();
}

////////////////////////////////////////////////////////////////////////////////
// set input file

void GEMReplay::SetInputFile(const std::string &f)
{
    ifile = f;
}

////////////////////////////////////////////////////////////////////////////////
// set pedestal output file

void GEMReplay::SetPedestalOutputFile(const std::string &f)
{
    pedestal_output_file = f;
}

////////////////////////////////////////////////////////////////////////////////
// set pedestal input file

void GEMReplay::SetPedestalInputFile(const std::string &f, const std::string &c)
{
    pedestal_input_file = f;
    common_mode_input_file = c;
}

////////////////////////////////////////////////////////////////////////////////
// set common mode file

void GEMReplay::SetCommonModeOutputFile(const std::string &f)
{
    commonMode_output_file = f;
}

////////////////////////////////////////////////////////////////////////////////
// set file split max

void GEMReplay::SetSplitMax(const int &s)
{
    split_max = s;
}

////////////////////////////////////////////////////////////////////////////////
// set file split min

void GEMReplay::SetSplitMin(const int &s)
{
    split_min = s;
}

////////////////////////////////////////////////////////////////////////////////
// set max pedestal events

void GEMReplay::SetMaxPedestalEvents(const int &s)
{
    max_pedestal_events = s;
}

////////////////////////////////////////////////////////////////////////////////
// get gem system

GEMSystem* GEMReplay::GetGEMSystem()
{
    return gem_sys;
}

////////////////////////////////////////////////////////////////////////////////
// get gem data handler

GEMDataHandler* GEMReplay::GetGEMDataHandler()
{
    return data_handler;
}



