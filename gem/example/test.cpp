#include "GEMPedestal.h"

#include <iostream>

#include <TH1I.h>
#include <TFile.h>

////////////////////////////////////////////////////////////////
// An example for how to use the raw event decoder

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    GEMPedestal *pedestal = new GEMPedestal();

    // generate pedestal
    pedestal->SetDataFile("../../../gem_cleanroom_2252.evio.0");
    pedestal->CalculatePedestal();
    pedestal->SavePedestalHisto("pedestal_0.root");

    // load pedestal
    //pedestal -> LoadPedestalHisto("pedestal_0.root");

    return 0;
}
