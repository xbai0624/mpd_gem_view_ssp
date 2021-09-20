#include "PreAnalysis.h"
#include <TGraphErrors.h>
#include <TMath.h>
#include <cassert>
#include <TFile.h>
#include <algorithm>
#include <map>

PreAnalysis* PreAnalysis::instance = nullptr;

////////////////////////////////////////////////////////////////////////////////

void PreAnalysis::UpdateEvent(const EventData &ev)
{
    const std::vector<GEM_Strip_Data> &gem_strip_data
        = ev.get_gem_data();

    for(auto &i: gem_strip_data)
    {
        // per APV
        APVAddress ad(i.addr.crate, i.addr.mpd, i.addr.adc);
        if(timeSampleAPVCheck.find(ad) == timeSampleAPVCheck.end()) 
        {
            for(size_t ts=0;ts<i.values.size();ts++) {
                timeSampleAPVCheck[ad].push_back((i.values)[ts]);
            }
            apvEntries[ad] = 1.0;
        } else {
            for(size_t ts=0;ts<i.values.size();ts++) {
                (timeSampleAPVCheck[ad])[ts] += (i.values)[ts];
            }
            apvEntries[ad] += 1.0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void PreAnalysis::SavePlots()
{
    // get average
    for(auto &i: timeSampleAPVCheck)
    {
        for(auto &j: i.second)
            j/=apvEntries[i.first];
    }

    // sort
    std::map<APVAddress, std::vector<double>> _timeSampleAPVCheck;
    for(auto &i: timeSampleAPVCheck)
        _timeSampleAPVCheck[i.first] = i.second;

    // save to file
    TFile *f = new TFile("Rootfiles/apv_time_sample_check.root", "recreate");
    for(auto &i: _timeSampleAPVCheck)
    {
        TGraphErrors *g = Plot(i.first, i.second);
        f->Append(g);
    }
    f->Write();
    f->Close();
}

////////////////////////////////////////////////////////////////////////////////

TGraphErrors *PreAnalysis::Plot(const APVAddress &addr, const std::vector<double> &apv_data)
{
    // time sample = 6
    assert(apv_data.size() == 6);

    //double x[6], y[6], xerr[6], yerr[6];
    double *x = new double[6], *y = new double[6], *xerr = new double[6],
    *yerr = new double[6];

    for(int i=0;i<6;i++){
        x[i] = i+1;
        y[i] = apv_data[i];
        xerr[i] = 0;
        yerr[i] = TMath::Sqrt(y[i]);
    }

    TGraphErrors *g = new TGraphErrors(6, x, y, xerr, yerr);
    std::string title = Form("g_crate_%d_mpd_%d_adc_%d", addr.crate_id, addr.mpd_id, addr.adc_ch);
    g->SetTitle(title.c_str());
    g->SetMarkerStyle(20);

    delete []x; delete []y; delete []xerr; delete []yerr;

    return g;
}

