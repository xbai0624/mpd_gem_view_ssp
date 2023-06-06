#include "standard_align.h"
#include "matrix.h"
#include "ToyModel.h"
#include <iostream>

using namespace tracking_dev;

int main(int argc, char* argv[])
{
    //ToyModel *toy = new ToyModel();
    //toy -> Generate();

    StandardAlign *align = new StandardAlign();
    align -> SetupToyModel();
    align -> SetNlayer(5);
    // align -> LoadTextFile("alignment/ref.txt");
    align -> SetAnchorLayers(0); // fix alignment for layers ...
    align -> Solve();
    align -> WriteTextFile("alignment/new_results.txt");

    return 0;
}
