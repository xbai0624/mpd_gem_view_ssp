
#include "WfRootGraph.h"

using namespace fdec;

#ifdef __cplusplus
extern "C" {
#endif

WfRootGraph get_waveform_graph(const Analyzer &ana, const std::vector<uint32_t> &samples, bool show_tspec,
                               double shade_alpha)
{
    WfRootGraph res;

    res.mg = new TMultiGraph();
    if (samples.empty()) {
        auto gr = new TGraph;
        gr->SetPoint(0, 0, 0);
        res.mg->Add(gr, "l");
        res.objs.push_back(dynamic_cast<TObject*>(gr));
        return res;
    }

    // raw waveform
    auto wf = new TGraph(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        wf->SetPoint(i, i, samples[i]);
    }
    wf->SetLineColor(kRed + 1);
    wf->SetLineWidth(1);
    wf->SetLineStyle(2);
    res.mg->Add(wf, "l");
    res.objs.push_back(dynamic_cast<TObject*>(wf));
    res.entries.emplace_back(LegendEntry{wf, "Raw Samples", "l"});

    // smoothed waveform
    auto wf2 = new TGraph(samples.size());
    auto buf = ana.SmoothSpectrum(&samples[0], samples.size(), ana.GetResolution());
    for (size_t i = 0; i < buf.size(); ++i) {
        wf2->SetPoint(i, i, buf[i]);
    }
    wf2->SetLineColor(kBlue + 1);
    wf2->SetLineWidth(2);
    res.mg->Add(wf2, "l");
    res.objs.push_back(dynamic_cast<TObject*>(wf2));
    res.entries.emplace_back(LegendEntry{wf2, Form("Smoothed Samples (res = %zu)", ana.GetResolution()), "l"});

    // peak finding
    res.result = ana.Analyze(&samples[0], samples.size());
    auto ped = res.result.ped;
    auto peaks = res.result.peaks;
    auto grm_pos = new TGraph();
    auto grm_neg = new TGraph();

    for (size_t i = 0; i < peaks.size(); ++i) {
        // peak amplitudes
        auto peak = peaks[i];
        double range = wf->GetYaxis()-> GetXmax() - wf->GetYaxis()-> GetXmin();
        double height = peak.height + ped.mean + (peak.height > 0 ? 1. : -1.5)*range*0.02;
        if (peak.height >= 0) {
            grm_pos->SetPoint(grm_pos->GetN(), peak.pos, height);
        } else {
            grm_neg->SetPoint(grm_neg->GetN(), peak.pos, height);
        }

        // peak integrals
        auto color = i + 2;
        auto nint = peak.right - peak.left + 1;
        auto grs = new TGraph(2*nint);
        for (size_t i = 0; i < nint; ++i) {
            auto val = buf[i + peak.left];
            grs->SetPoint(i, i + peak.left, val);
            grs->SetPoint(nint + i, peak.right - i, ped.mean);
        }
        grs->SetFillColorAlpha(color, shade_alpha);
        grs->SetFillStyle(3002);
        res.mg->Add(grs, "f");
        res.objs.push_back(dynamic_cast<TObject*>(grs));
        if (i == 0) {
            res.entries.emplace_back(LegendEntry{grs, "Peak Integrals", "f"});
        }
    }
    grm_pos->SetMarkerStyle(23);
    grm_pos->SetMarkerSize(1.0);
    if (grm_pos->GetN()) { res.mg->Add(grm_pos, "p"); }
    res.objs.push_back(dynamic_cast<TObject*>(grm_pos));
    res.entries.emplace_back(LegendEntry{grm_pos, "Peaks", "p"});

    grm_neg->SetMarkerStyle(22);
    grm_neg->SetMarkerSize(1.0);
    if (grm_neg->GetN()) { res.mg->Add(grm_neg, "p"); }
    res.objs.push_back(dynamic_cast<TObject*>(grm_neg));

    // pedestal line
    auto grp = new TGraphErrors(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        grp->SetPoint(i, i, ped.mean);
        grp->SetPointError(i, 0, ped.err);
    }
    grp->SetFillColorAlpha(kBlack, shade_alpha);
    grp->SetFillStyle(3002);
    grp->SetLineWidth(2);
    grp->SetLineColor(kBlack);
    res.mg->Add(grp, "l3");
    res.objs.push_back(dynamic_cast<TObject*>(grp));
    res.entries.emplace_back(LegendEntry{grp, "Pedestal", "lf"});

    // TSpectrum background
    if (show_tspec) {
        std::vector<double> tped(samples.size());
        tped.assign(samples.begin(), samples.end());
        TSpectrum s;
        s.Background(&tped[0], samples.size(), samples.size()/4, TSpectrum::kBackDecreasingWindow,
                     TSpectrum::kBackOrder2, false, TSpectrum::kBackSmoothing3, false);
        auto grp2 = new TGraph(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            grp2->SetPoint(i, i, tped[i]);
        }
        grp2->SetLineStyle(2);
        grp2->SetLineWidth(2);
        grp2->SetLineColor(kBlack);
        res.mg->Add(grp2, "l");
        res.objs.push_back(dynamic_cast<TObject*>(grp2));
        res.entries.emplace_back(LegendEntry{grp2, "TSpectrum Background", "l"});
    }

    return res;
}



#ifdef __cplusplus
}
#endif


