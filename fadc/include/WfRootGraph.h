#pragma once

#include "WfAnalyzer.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TSpectrum.h"
#include "TStyle.h"
#include "TAxis.h"
#include <algorithm>
#include <string>
#include <vector>
#include <random>


namespace fdec {

struct LegendEntry
{
    TObject *obj;
    std::string label, option;
};

class WfRootGraph
{
public:
    TMultiGraph *mg;
    std::vector<TObject*> objs;
    std::vector<LegendEntry> entries;
    Fadc250Data result;
};

extern "C" {
WfRootGraph get_waveform_graph(const Analyzer &ana, const std::vector<uint32_t> &samples, bool show_tspec = false,
                               double shade_alpha = 0.3);
}

} // namespace fdec
