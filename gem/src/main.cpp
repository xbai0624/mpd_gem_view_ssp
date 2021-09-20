#include <iostream>

#include "GEMSystem.h"
#include "GEMDataHandler.h"

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[])
{
    GEMDataHandler *data_handler = new GEMDataHandler();

    [[maybe_unused]]GEMSystem *gem_sys = new GEMSystem();
    gem_sys -> Configure("config/gem.conf");
    //gem_sys -> SetPedestalMode(true);

    data_handler -> SetGEMSystem(gem_sys);

    //std::string ifile = "../../gem_cleanroom_2252.evio.0";
    std::string ifile = "../../gem_cleanroom_2069.evio.47";
    std::string ofile = "gem_replay.root";
    int split = -1;
    data_handler -> Replay(ifile, split, ofile);

    return 0;
}
