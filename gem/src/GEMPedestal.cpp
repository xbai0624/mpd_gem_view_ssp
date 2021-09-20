////////////////////////////////////////////////////////////////
// Algorithm for pedestal calculation is arguable
// If one wants to discuss it, 
// please email Xinzhan Bai @ xb4zp@virginia.edu
// -------- 11/13/2020

#include "GEMPedestal.h"
#include "MPDVMERawEventDecoder.h"
#include "MPDSSPRawEventDecoder.h"
#include "hardcode.h"

#include <iostream>
#include <thread>
#include <mutex>

#include <TFile.h>
#include <TROOT.h>
#include <TKey.h>


////////////////////////////////////////////////////////////////
// if computing resource is limited, then turn off multi-threading
// by setting NTHREAD to 1

#define NTHREAD 3
std::mutex mtx;

////////////////////////////////////////////////////////////////
// ctor

GEMPedestal::GEMPedestal()
{
    // place holder
}

////////////////////////////////////////////////////////////////
// dtor

GEMPedestal::~GEMPedestal()
{
    Clear();
}

////////////////////////////////////////////////////////////////
// set file used for calculating pedestal

void GEMPedestal::SetDataFile(const char* path)
{
    data_file_path = path;
}

////////////////////////////////////////////////////////////////
// set total number of events used for calculating pedestal

void GEMPedestal::SetNumberOfEvents(int num)
{
    fNumberEvents = static_cast<uint32_t>(num);
}

////////////////////////////////////////////////////////////////
// get total number of events used for calculating pedestal

int GEMPedestal::GetNumberOfEvents() const
{
    return static_cast<int>(fNumberEvents);
}

////////////////////////////////////////////////////////////////
// calculate pedestal

void GEMPedestal::CalculatePedestal()
{
    if(!file_reader)
        file_reader = new EvioFileReader();
    else
        file_reader -> CloseFile();

    file_reader -> SetFile(data_file_path);
    file_reader -> OpenFile();

    EventParser *event_parser[NTHREAD];
#ifdef USE_VME
    MPDVMERawEventDecoder *mpd_decoder[NTHREAD];
#else
    MPDSSPRawEventDecoder *mpd_decoder[NTHREAD];
#endif

    for(int i=0;i<NTHREAD;i++){
        event_parser[i] = new EventParser();
#ifdef USE_VME
        mpd_decoder[i] = new MPDVMERawEventDecoder();
        event_parser[i]->RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_VME), mpd_decoder[i]);
#else
        mpd_decoder[i] = new MPDSSPRawEventDecoder();
        event_parser[i]->RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP), mpd_decoder[i]);
#endif
    }

    uint32_t nEvents = 0;

    std::vector<std::thread> vth;
    for(int i=0;i<NTHREAD;i++){
        vth.emplace_back(&GEMPedestal::GetEvent, this, file_reader, event_parser[i], std::ref(nEvents));
    }

    for(auto &i: vth)
        i.join();

    //GenerateAPVPedestal_using_histo(); // slow
    GenerateAPVPedestal_using_vec();     // fast
}

////////////////////////////////////////////////////////////////
// precess batch events

void GEMPedestal::GetEvent(EvioFileReader *file_reader, EventParser *event_parser,
        uint32_t &nEvents)
{
    const uint32_t *pBuf;
    uint32_t fBufLen;
    bool run = true;
    while(run)
    {
        mtx.lock();
        run = (file_reader -> ReadNoCopy(&pBuf, &fBufLen)) == S_SUCCESS;
        nEvents++;
        mtx.unlock();

        event_parser->ParseEvent(pBuf, fBufLen);
#ifdef USE_VME
        [[maybe_unused]] auto & decoded_data = dynamic_cast<MPDVMERawEventDecoder*>(
                event_parser->GetRawDecoder(static_cast<int>(Bank_TagID::MPD_VME)))
            ->GetAPV();
#else
        [[maybe_unused]] auto & decoded_data = dynamic_cast<MPDSSPRawEventDecoder*>(
                event_parser->GetRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP)))
            ->GetAPV();
#endif

        CalculateEventRawPedestal(decoded_data);

        if(nEvents >= fNumberEvents)
            break;
    }
}

////////////////////////////////////////////////////////////////
// calculate raw pedestal for one event

void GEMPedestal::CalculateEventRawPedestal(
        const std::unordered_map<APVAddress, std::vector<int>> & event_data)
{
    for(auto &i: event_data)
    {
        //RawAPVUnit_histo(i); // slow
        RawAPVUnit_vec(i);     // fast
    }

    //RawPedestalThread(event_data, 0, static_cast<int>(event_data.size()));
}

////////////////////////////////////////////////////////////////
// process a sub range of the current event, for parallel

void GEMPedestal::RawPedestalThread(const std::unordered_map<APVAddress, std::vector<int>> & data,
        int beg, int end)
{
    std::unordered_map<APVAddress, std::vector<int>>::const_iterator it_beg = data.begin();
    std::unordered_map<APVAddress, std::vector<int>>::const_iterator it_end = data.begin();
    std::advance(it_beg, beg);
    std::advance(it_end, end);

    for(;it_beg!=it_end;it_beg++)
    {
        RawAPVUnit_histo(*it_beg);
    }
}


////////////////////////////////////////////////////////////////
// process raw data in one APV, using TH1I (slow)

void GEMPedestal::RawAPVUnit_histo(const std::unordered_map<APVAddress, std::vector<int>>::value_type & i)
{
    const std::vector<StripRawADC> & apv_raw_data = DecodeAPV(i.second);
    auto apv_ts_commonMode = GetTimeSampleCommonMode(apv_raw_data);

    for(auto &strip: apv_raw_data)
    {
        APVStripAddress addr(i.first, strip.stripNo);

        if(mAPVStripNoise.find(addr) == mAPVStripNoise.end())
        {
            mtx.lock();
            mAPVStripNoise[addr] = new TH1I(
                    Form("noise_crate_%d_mpd_%d_ch_%d_strip_%d", addr.crate_id, addr.mpd_id, addr.adc_ch, addr.strip_no), 
                    "strip noise", 800, -400, 400
                    );
            mAPVStripOffset[addr] = new TH1I(
                    Form("offset_crate_%d_mpd_%d_ch_%d_strip_%d", addr.crate_id, addr.mpd_id, addr.adc_ch, addr.strip_no), 
                    "strip offset", 1000, 400, 1400
                    );
            mtx.unlock();
        }

        int time_sample_size = strip.GetTimeSampleSize();

        int offset = 0;
        for(auto &adc: strip.v_adc)
            offset += adc;
        offset /= time_sample_size;

        int noise = 0;
        for(int ts = 0; ts<time_sample_size;ts++)
            noise += (strip.v_adc[ts] - apv_ts_commonMode[ts]);
        noise /= time_sample_size;

        mtx.lock();
        mAPVStripNoise[addr] -> Fill(noise);
        mAPVStripOffset[addr] -> Fill(offset); 
        mtx.unlock();
    }
}

////////////////////////////////////////////////////////////////
// process raw data in one APV, using std::vector (fast)

void GEMPedestal::RawAPVUnit_vec(const std::unordered_map<APVAddress, std::vector<int>>::value_type & i)
{
    const std::vector<StripRawADC> & apv_raw_data = DecodeAPV(i.second);
    auto apv_ts_commonMode = GetTimeSampleCommonMode(apv_raw_data);

    for(auto &strip: apv_raw_data)
    {
        APVStripAddress addr(i.first, strip.stripNo);

        int time_sample_size = strip.GetTimeSampleSize();

        int offset = 0;
        for(auto &adc: strip.v_adc)
            offset += adc;
        offset /= time_sample_size;

        int noise = 0;
        for(int ts = 0; ts<time_sample_size;ts++)
            noise += (strip.v_adc[ts] - apv_ts_commonMode[ts]);
        noise /= time_sample_size;

        mtx.lock();
        mAPVStripNoiseVec[addr].push_back(noise);
        mAPVStripOffsetVec[addr].push_back(offset); 
        mtx.unlock();
    }
}


////////////////////////////////////////////////////////////////
// get common Mode for each time sample in one APV

std::vector<int> GEMPedestal::GetTimeSampleCommonMode(const std::vector<StripRawADC> &apv_data)
{
    if(apv_data.size() != APV_STRIP_SIZE) {
        std::cout<<"Error: incorrect apv channel number."
            <<std::endl;
        exit(0);
    }

    int time_sample = apv_data[0].GetTimeSampleSize();

    // common mode in each time sample
    std::vector<int> res(time_sample, 0); 

    for(auto &strip: apv_data)
    {
        for(int ts=0;ts<time_sample;ts++) {
            res[ts] += strip.v_adc[ts];
        }
    }

    // common mode
    for(int ts = 0; ts<time_sample;ts++)
        res[ts] /= APV_STRIP_SIZE;

    return res;
}

////////////////////////////////////////////////////////////////
// generate pedestal for each APV using the strip noise and offset
// using TH1I (slow)

void GEMPedestal::GenerateAPVPedestal_using_histo()
{
    // generate noise
    for(auto &i: mAPVStripNoise) {
        APVAddress addr(i.first.crate_id, i.first.mpd_id, i.first.adc_ch);
        int strip_no = i.first.strip_no;

        if(mAPVNoiseHisto.find(addr) == mAPVNoiseHisto.end())
            mAPVNoiseHisto[addr] = new TH1I(
                    Form("noise_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    Form("noise_crate_%d_mpd_%d_ch_%d_RMS", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    APV_STRIP_SIZE+20, -10, APV_STRIP_SIZE+10);

        int noise = i.second->GetRMS();
        mAPVNoiseHisto[addr] -> SetBinContent(strip_no+10, noise);
        mAPVNoise[addr].push_back(noise);
    }

    // generate offset
    for(auto &i: mAPVStripOffset) {
        APVAddress addr(i.first.crate_id, i.first.mpd_id, i.first.adc_ch);
        int strip_no = i.first.strip_no;

        if(mAPVOffsetHisto.find(addr) == mAPVOffsetHisto.end())
            mAPVOffsetHisto[addr] = new TH1I(
                    Form("offset_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    Form("offset_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    APV_STRIP_SIZE+20, -10, APV_STRIP_SIZE+10);

        int offset = i.second -> GetMean();
        mAPVOffsetHisto[addr] -> SetBinContent(strip_no + 10, offset);
        mAPVOffset[addr].push_back(offset);
    }
}

////////////////////////////////////////////////////////////////
// generate pedestal for each APV using the strip noise and offset
// using vector (fast)

void GEMPedestal::GenerateAPVPedestal_using_vec()
{
    // overall noise distribution
    hOverallNoiseHisto = new TH1I("hOverallNoise", "RMS Noise", 400, 0, 400);

    // generate noise for each apv
    for(auto &i: mAPVStripNoiseVec) {
        APVAddress addr(i.first.crate_id, i.first.mpd_id, i.first.adc_ch);
        int strip_no = i.first.strip_no;

        if(mAPVNoiseHisto.find(addr) == mAPVNoiseHisto.end())
            mAPVNoiseHisto[addr] = new TH1I(
                    Form("noise_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    Form("noise_crate_%d_mpd_%d_ch_%d_RMS", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    APV_STRIP_SIZE+20, -10, APV_STRIP_SIZE+10
                    );

        int noise = GetRMS(i.second);
        mAPVNoiseHisto[addr] -> SetBinContent(strip_no+10, noise);
        mAPVNoise[addr].push_back(noise);
        hOverallNoiseHisto -> Fill(noise);
    }

    // generate offset
    for(auto &i: mAPVStripOffsetVec) {
        APVAddress addr(i.first.crate_id, i.first.mpd_id, i.first.adc_ch);
        int strip_no = i.first.strip_no;

        if(mAPVOffsetHisto.find(addr) == mAPVOffsetHisto.end())
            mAPVOffsetHisto[addr] = new TH1I(
                    Form("offset_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    Form("offset_crate_%d_mpd_%d_ch_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                    APV_STRIP_SIZE+20, -10, APV_STRIP_SIZE+10
                    );

        int offset = GetMean(i.second);
        mAPVOffsetHisto[addr] -> SetBinContent(strip_no + 10, offset);
        mAPVOffset[addr].push_back(offset);
    }
}

////////////////////////////////////////////////////////////////
// a helper: get mean of a vector (for offset calculation)

int GEMPedestal::GetMean(const std::vector<int> &data)
{
    TH1I h("h", "h", 1000, 400, 1400);
    for(auto &i: data)
        h.Fill(i);

    int res = h.GetMean();
    return res;
}

////////////////////////////////////////////////////////////////
// a helper: get RMS of a vector (for noise calculation)

int GEMPedestal::GetRMS(const std::vector<int> &data)
{
    TH1I h("h", "h", 800, -400, 400);
    for(auto &i: data)
        h.Fill(i);

    int res = h.GetRMS();
    return res;
}


////////////////////////////////////////////////////////////////
// decode raw apv data 

std::vector<StripRawADC> GEMPedestal::DecodeAPV(std::vector<int> const & apv_data)
{
    std::vector<StripRawADC> res;

    int event_size = static_cast<int>(apv_data.size());
    if(event_size % MPD_APV_TS_LEN != 0) {
        std::cout<<"Warning: apv data size incorrect. Data might be corrupted."
            <<std::endl;
        return res;
    }
    int nTimeSample = event_size / MPD_APV_TS_LEN;

    for(int i=0; i<APV_STRIP_SIZE; i++) 
    {
        StripRawADC strip_raw_adc;
        strip_raw_adc.stripNo = i;

        for(int ts=0;ts<nTimeSample;ts++) {
            strip_raw_adc.v_adc.push_back(apv_data[i + ts*MPD_APV_TS_LEN]);
        }

        res.push_back(strip_raw_adc);
    }

    return res;
}

////////////////////////////////////////////////////////////////
// get pedestal noise

const std::unordered_map<APVAddress, std::vector<int>> & GEMPedestal::
GetAPVNoise() const
{
    return mAPVNoise;
}

////////////////////////////////////////////////////////////////
// get pedestal offset

const std::unordered_map<APVAddress, std::vector<int>> & GEMPedestal::
GetAPVOffset() const
{
    return mAPVOffset;
}

////////////////////////////////////////////////////////////////
// get pedestal noise histos

const std::unordered_map<APVAddress, TH1I*> & GEMPedestal::
GetAPVNoiseHisto() const 
{
    return mAPVNoiseHisto;
}

////////////////////////////////////////////////////////////////
// get pedestal offset histos

const std::unordered_map<APVAddress, TH1I*> & GEMPedestal::
GetAPVOffsetHisto() const
{
    return mAPVOffsetHisto;
}

////////////////////////////////////////////////////////////////
// save pedestal histograms to disk

void GEMPedestal::SavePedestalHisto(const char* path)
{
    TFile *f = new TFile(path, "recreate");
    for(auto &i: mAPVNoiseHisto) {
        i.second->SetDirectory(f);
        i.second->GetXaxis()->SetTitle("APV Channel No.");
        i.second->GetYaxis()->SetTitle("ADC");
        i.second->Write();
    }
    for(auto &i: mAPVOffsetHisto) {
        i.second->GetXaxis()->SetTitle("APV Channel No.");
        i.second->GetYaxis()->SetTitle("ADC");
        i.second->SetDirectory(f);
        i.second-> Write();
    }
    hOverallNoiseHisto -> Write();
    f->Close();
}

////////////////////////////////////////////////////////////////
// save pedestal texts to disk

void GEMPedestal::SavePedestalText()
{
}

////////////////////////////////////////////////////////////////
// load pedestal histograms to momory

void GEMPedestal::LoadPedestalHisto(const char* path)
{
    TFile *f = TFile::Open(path);
    TIter keyList(f->GetListOfKeys());
    TKey *key;
    while((key = (TKey*)keyList()))
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;
        TH1I* h = (TH1I*)key->ReadObj();
        std::string name = h->GetName();
        auto apv_address = ParseAPVAddressFromString(name);
        if(name.find("noise") != std::string::npos)
            mAPVNoiseHisto[apv_address] = h;
        else if(name.find("offset") != std::string::npos)
            mAPVOffsetHisto[apv_address] = h;
    }
}

////////////////////////////////////////////////////////////////
// load pedestal texts to memory

void GEMPedestal::LoadPedestalText()
{
}

////////////////////////////////////////////////////////////////
// parse apv address from string, when loading pedstal from file

APVAddress GEMPedestal::ParseAPVAddressFromString(const std::string & s)
{
    // noise_crate_%d_mpd_%d_ch_%d
    auto search = [&](size_t pos, const std::string & str, char tok)
        -> size_t
        {
            size_t res = -1;
            for(size_t i = pos;i<str.size(); i++){
                res = i;
                char _s = static_cast<char>(str[i]);
                if(_s == tok) return res;
            }
            return res;
        };

    // crate id
    size_t pos = s.find("crate_");
    size_t pos_ = search(pos + 6, s, '_');
    int crate_id = stoi(s.substr(pos+6, pos_));
    // mpd id
    pos = s.find("mpd_");
    pos_ = search(pos+4, s, '_');
    int mpd_id = stoi(s.substr(pos+4, pos_));
    // adc channel
    pos = s.find("ch_");
    pos_ = search(pos+3, s, '_');
    int adc_ch = stoi(s.substr(pos+3, pos_));

    APVAddress res(crate_id, mpd_id, adc_ch);
    return res;
}


////////////////////////////////////////////////////////////////
// reset

void GEMPedestal::Clear()
{
    for(auto &i: mAPVNoiseHisto)
        if(i.second) i.second->Delete();

    for(auto &i: mAPVOffsetHisto)
        if(i.second) i.second->Delete();

    for(auto &i: mAPVStripNoise)
        if(i.second) i.second->Delete();

    for(auto &i: mAPVStripOffset)
        if(i.second) i.second->Delete();

    for(auto &i: mAPVStripNoiseVec)
        i.second.clear();

    for(auto &i: mAPVStripOffsetVec)
        i.second.clear();
}


////////////////////////////////////////////////////////////////
// a thread-safe unordered_map look up routine
// when one thread is looking up the map, other threads are not
// allowed to modify the map

bool GEMPedestal::APVStripIsNew(const APVStripAddress & addr)
{
    mtx.lock();
    bool res = mAPVStripNoise.find(addr) == mAPVStripNoise.end();
    mtx.unlock();

    return res;
}


