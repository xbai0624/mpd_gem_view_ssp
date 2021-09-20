#pragma once

// A waveform analyzer class for the Fadc250 raw waveform data
// It finds the local maxima (peaks) over the spectrum, and detemines the peak properties
// It is intended to use simple algorithms since we do not expect heavily convoluted spectra
//
// History:
// v1.0.0: a working version with basic functionalities, Chao Peng, 2020/08/04

#include "Fadc250Data.h"
#include <algorithm>
#include <vector>
#include <cmath>


namespace fdec {

// some help functions
// calculate mean and standard deviation of an array
template<typename T>
inline void _calc_mean_err(double &mean, double &err, const T *source, size_t npts)
{
    if (npts == 0) { return; }

    mean = 0.;
    err = 0.;

    for (size_t i = 0; i < npts; ++i) {
       mean += source[i];
    }
    mean /= static_cast<double>(npts);

    for (size_t i = 0; i < npts; ++i) {
       err += (source[i] - mean ) * (source[i] - mean);
    }
    err = std::sqrt(err/static_cast<double>(npts));
}

template<typename T>
inline void _linear_regr(double &p0, double &p1, double &err, const T *x, const T *y, size_t npts)
{
    // no data points
    if (npts == 0) {
        return;
    // too few data points
    } else if (npts < 3) {
        p1 = 0.;
        _calc_mean_err(p0, err, y, npts);
        return;
    }

    double x_mean, x_err;
    _calc_mean_err(x_mean, x_err, x, npts);
    double y_mean, y_err;
    _calc_mean_err(y_mean, y_err, y, npts);

    double xysum = 0.;
    for (size_t i = 0; i < npts; ++i) {
        xysum += (x[i] - x_mean) * (y[i] - y_mean);
    }

    double r = xysum/(x_err*y_err*npts);

    p1 = r*y_err/x_err;
    p0 = y_mean - p1*x_mean;

    err = 0.;
    for (size_t i = 0; i < npts; ++i) {
        double diff = (p0 + p1*x[i] - y[i]);
        err += diff*diff;
    }
    err = std::sqrt(err/static_cast<double>(npts));
}

// analyzer class
class Analyzer
{
public:
    // clk is in MHz
    Analyzer(size_t resolution = 2, double threshold = 10.0,
             size_t n_ped_samples = 5, double ped_flatness = 1.0,
             uint32_t max_value = 4096, double clk = 250.);
    virtual ~Analyzer() {}

    // analyze waveform samples
    void Analyze(Fadc250Data &data) const;
    Fadc250Data Analyze(const uint32_t *samples, size_t nsamples) const;

    // find pedestal, it assumes the pedestal is a constant (for simple FADC250 spectrum)
    Pedestal FindPedestal(const std::vector<double> &buffer, const std::vector<Peak> &/*peaks*/) const;

    // search local maxima as peak candidates
    std::vector<Peak> SearchMaxima(const std::vector<double> &buffer, double height_thres) const;

    // get
    double GetThreshold() const { return _thres; }
    double GetClockFreq() const { return _clk; }
    size_t GetResolution() const { return _res; }
    size_t GetNSamplesPed() const { return _npeds; }
    size_t GetOverflowValue() const { return _overflow; }

    // set
    void SetThreshold(double thres) { _thres = thres; }
    void SetClockFreq(double clk) { _clk = clk; }
    void SetResolution(size_t res) { _res = res; }
    void SetNSamplesPed(size_t npeds) { _npeds = npeds; }
    void SetOverflowValue(uint32_t overflow) { _overflow = overflow; }

private:
    double _thres, _clk, _ped_flat;
    size_t _res, _npeds;
    uint32_t _overflow;


public:
    // static methods
    template<typename T>
    static std::vector<double> SmoothSpectrum(const T *samples, size_t nsamples, size_t res)
    {
        if (res <= 1) {
            return std::vector<double>(samples, samples + nsamples);
        }
        std::vector<double> buffer(nsamples);
        for (size_t i = 0; i < nsamples; ++i) {
            double val = samples[i];
            double weights = 1.0;
            for (size_t j = 1; j < res; ++j) {
                if (j >= i || j + i >= nsamples) { continue; }
                double weight = 1.0 - j/static_cast<double>(res + 1);
                val += weight*(samples[i - j] + samples[i + j]);
                weights += 2.*weight;
            }
            buffer[i] = val/weights;
        }
        return buffer;
    }

    template<typename T>
    static Pedestal CalcPedestal(T *ybuf, size_t npts, double thres = 1.0, int max_iters = 3, size_t min_npeds = 5)
    {
        Pedestal res;
        _calc_mean_err(res.mean, res.err, ybuf, npts);
        // iteratively fit
        while (max_iters-- > 0) {
            size_t count = 0;
            for (size_t i = 0; i < npts; ++i) {
                if (std::abs(ybuf[i] - res.mean) < res.err*thres) {
                    ybuf[count] = ybuf[i];
                    count++;
                }
            }
            if ((count == npts) || (count < min_npeds)) { break; }
            npts = count;
            _calc_mean_err(res.mean, res.err, ybuf, npts);
        }

        return res;
    }

};  // class Analyzer

};  // namespace fdec

