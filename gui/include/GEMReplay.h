/*
 * This class is the future class, it is planned to replace GEMAnalyzer.
 *
 * At present, GEMAnalyzer is a concise class dedicated for pedestal and
 * raw data processing, and has multithreading enabled. GEMAnalyzer is 
 * much faster, but independent of the gem code base.
 *
 * This class can do everything that GEMAnalyzer does, but multithreading 
 * was disabled. This class directly use gem code base.
 *
 */

#ifndef GEM_REPLAY_H
#define GEM_REPLAY_H

#include "GEMSystem.h"
#include "GEMDataHandler.h"
#include "APVStripMapping.h"
#include "ConfigObject.h"

class GEMReplay
{
public:
    GEMReplay();
    ~GEMReplay();

    void GeneratePedestal();
    void ReplayHit();
    void ReplayCluster();

    // setters
    void SetInputFile(const std::string &);
    void SetPedestalOutputFile(const std::string &);
    void SetPedestalInputFile(const std::string &, const std::string &);
    void SetCommonModeOutputFile(const std::string &);
    void SetSplitMax(const int &s);
    void SetSplitMin(const int &s);
    void SetMaxPedestalEvents(const int &s);

    // getters
    GEMSystem* GetGEMSystem();
    GEMDataHandler *GetGEMDataHandler();

private:
    GEMDataHandler *data_handler;
    GEMSystem *gem_sys;

    std::string ifile; // input file
    std::string pedestal_output_file = "database/gem_ped.dat"; // pedestal file
    std::string commonMode_output_file = "database/CommonModeRange.txt"; // common mode file
    std::string pedestal_input_file = "database/gem_ped.dat"; // pedestal file
    std::string common_mode_input_file = "database/CommonModeRange.txt"; // common mode input file
    int split_max = -1;
    int split_min = 0;
    int max_pedestal_events = -1;

    ConfigObject txt_parser;
};

#endif
