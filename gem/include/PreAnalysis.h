#ifndef PRE_ANALYSIS_H
#define PRE_ANALYSIS_H

/*
 * this class is for generating preliminary data quality check plots,
 * it plots apv time sample average adc information.
 *
 * This is to Thir's request, it is independent of the core package,
 * there's no need for you to read it
 *  
 */

#include "GEMStruct.h"

class TGraphErrors;

class PreAnalysis
{
public:
    static PreAnalysis* Instance() {
        if(!instance)
            instance = new PreAnalysis;
        return instance;
    }

    void UpdateEvent(const EventData &ev);
    void SavePlots();

    TGraphErrors *Plot(const APVAddress &addr, const std::vector<double> &apv_data);

private:
    static PreAnalysis *instance;
    PreAnalysis(){};

    std::unordered_map<APVAddress, std::vector<double>> timeSampleAPVCheck;
    std::unordered_map<APVAddress, double> apvEntries;
 
};

#endif
