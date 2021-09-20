#include "TSpectrum.h"
#include "WfAnalyzer.h"
#include <iostream>


using namespace fdec;

// constructor
Analyzer::Analyzer(size_t res, double thres, size_t npeds, double ped_flat, uint32_t overflow, double clk)
: _thres(thres), _clk(clk), _ped_flat(ped_flat), _res(res), _npeds(npeds), _overflow(overflow)
{
    // place holder
}

void Analyzer::Analyze(Fadc250Data &data) const
{
    uint32_t *samples = &data.raw[0];
    size_t nsamples = data.raw.size();
    if (!nsamples) { return; }

    data.peaks.clear();

    auto buffer = SmoothSpectrum(samples, nsamples, _res);

    // search local maxima
    auto candidates = SearchMaxima(buffer, _thres);

    // get pedestal
    data.ped = FindPedestal(buffer, candidates);

    // get final results
    for (auto &peak : candidates) {
        // pedestal subtraction
        double peak_height = buffer[peak.pos] - data.ped.mean;
        // wrong baselin in the rough candidtes finding, below threshold, or not statistically significant
        if ((peak_height * peak.height < 0.) ||
            (std::abs(peak_height) < _thres) ||
            (std::abs(peak_height) < 3.0*data.ped.err)) {
            continue;
        }
        peak.height = peak_height;

        // integrate it over the peak range
        peak.integral = buffer[peak.pos] - data.ped.mean;
        // i may go below 0
        for (int i = peak.pos - 1; i >= static_cast<int>(peak.left); --i) {
            double val = buffer[i] - data.ped.mean;
            // stop when it touches or acrosses the baseline
            if (std::abs(val) < data.ped.err || val * peak.height < 0.) {
                peak.left = i; break;
            }
            peak.integral += val;
        }
        for (size_t i = peak.pos + 1; i <= peak.right; ++i) {
            double val = buffer[i] - data.ped.mean;
            if (std::abs(val) < data.ped.err || val * peak.height < 0.) {
                peak.right = i; break;
            }
            peak.integral += val;
        }

        // determine the real sample peak
        uint32_t sample_pos = peak.pos;
        peak.height = samples[sample_pos] - data.ped.mean;
        auto update_peak = [] (Peak &peak, double val, uint32_t pos) {
            if (std::abs(val) > std::abs(peak.height)) {
                peak.pos = pos;
                peak.height = val;
            }
        };
        for (size_t i = 1; i < _res; ++i) {
            if (sample_pos > i) {
                update_peak(peak, samples[sample_pos - i] - data.ped.mean, sample_pos - i);
            }
            if (sample_pos + i < nsamples) {
                update_peak(peak, samples[sample_pos + i] - data.ped.mean, sample_pos + i);
            }
        }

        // time unit conversion (to ns)
        peak.time = peak.pos * 1e3/_clk;
        // overflow check
        peak.overflow = samples[peak.pos] >= _overflow;
        // fill to results
        data.peaks.emplace_back(peak);
    }

    /*
    std::sort(peaks.begin(), peaks.end(),
                [] (const Peak &p1, const Peak &p2) { return p1.height > p2.height; });
    */
    return;
}


// analyze waveform
Fadc250Data Analyzer::Analyze(const uint32_t *samples, size_t nsamples) const
{
    Fadc250Data data;

    // copy buffer
    data.raw.assign(samples, samples + nsamples);

    Analyze(data);

    return data;
}

//find pedestal, it assumes the pedestal is a constant (for simple FADC250 spectrum)
Pedestal Analyzer::FindPedestal(const std::vector<double> &buffer, const std::vector<Peak> &/*peaks*/) const
{
    Pedestal ped{0., 0.};
    // too few samples, use the minimum value as the pedestal
    if (buffer.size() < _npeds) {
        _calc_mean_err(ped.mean, ped.err, &buffer[0], buffer.size());
        for (auto &val : buffer) {
            if (val < ped.mean) { ped.mean = val; }
        }
        return ped;
    }

    // number of trailing samples for pedestal
    int ntrails = std::max(_npeds, buffer.size()/12);
    // criteria for good pedestal (some overflow events will have a few flat samples)
    double max_mean = _overflow*0.95;
    bool find_baseline = false;

    // progressively find a good baseline
    ped.mean = max_mean;
    for (size_t i = 0; i <= buffer.size() - ntrails; ++i) {
        double mean = 0., err = 100.*_ped_flat;
        _calc_mean_err(mean, err, &buffer[i], ntrails);
        if(err < _ped_flat && mean < max_mean) {
            find_baseline = true;
            if (mean < ped.mean) { ped.mean = mean; ped.err = err; }
        }
    }
    if (find_baseline) {
        return ped;
    }

    // complicated spectrum
    auto ybuf = buffer;
    TSpectrum s;
    s.Background(&ybuf[0], ybuf.size(), ybuf.size()/4, TSpectrum::kBackDecreasingWindow,
                    TSpectrum::kBackOrder2, false, TSpectrum::kBackSmoothing3, false);
    return CalcPedestal(&ybuf[ybuf.size()/5], 3*ybuf.size()/5, 1.0, 3, _npeds);
}

std::vector<Peak> Analyzer::SearchMaxima(const std::vector<double> &buffer, double height_thres) const
{
    std::vector<Peak> candidates;
    if (buffer.size() < 3) { return candidates; }

    candidates.reserve(buffer.size()/3);
    // get trend
    auto trend = [] (double v1, double v2, double thr = 0.1) {
        return std::abs(v1 - v2) < thr ? 0 : (v1 > v2 ? 1 : -1);
    };
    for (uint32_t i = 1; i < buffer.size() - 1; ++i) {
        int tr1 = trend(buffer[i], buffer[i - 1]);
        int tr2 = trend(buffer[i], buffer[i + 1]);
        // peak at the rising (declining) edge
        if ((tr1 * tr2 >= 0) && (std::abs(tr1) > 0)) {
            uint32_t left = 1, right = 1;
            // search the peak range
            while ((i > left + 1) && (trend(buffer[i - left], buffer[i - left - 1]) == tr1)) {
                left ++;
            }
            while ((i + right < buffer.size() - 1) && (trend(buffer[i + right], buffer[i + right + 1])*tr1 >= 0)) {
                right ++;
            }

            double base = (buffer[i - left] * right + buffer[i + right] * left) / static_cast<double>(left + right);
            double height = std::abs(buffer[i] - base);
            if (height > height_thres) {
                candidates.emplace_back(buffer[i] - base, 0., 0., i, i - left, i + right);
            }
        }
    }
    return candidates;
}

