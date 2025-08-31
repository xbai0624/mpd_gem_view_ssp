/*
 * test ssp decoder
 */

#include "EvioFileReader.h"
#include "EventParser.h"
#include "MPDVMERawEventDecoder.h"
#include "MPDSSPRawEventDecoder.h"

#include <iostream>

#include <TH1I.h>
#include <TFile.h>

////////////////////////////////////////////////////////////////
// An example for how to use the raw event decoder

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    // set up the evio file reader
    EvioFileReader *file_reader = new EvioFileReader();
    file_reader -> SetFile("../../../bbgem_94.evio.0");
    file_reader -> OpenFile();

    // set up the event parser
    EventParser *event_parser = new EventParser();

    // initialize all detector raw decoders
    MPDSSPRawEventDecoder *mpd_ssp_decoder = new MPDSSPRawEventDecoder();

    // register all detector raw decoders
    event_parser->RegisterRawDecoder(static_cast<int>(Bank_TagID::MPD_SSP), mpd_ssp_decoder);

    // read event to buffer
    const uint32_t *pBuf;
    uint32_t fBufLen;
    while(file_reader->ReadNoCopy(&pBuf, &fBufLen) == S_SUCCESS)
    {
        event_parser->ParseEvent(pBuf, fBufLen);

        [[maybe_unused]] auto & decoded_data = mpd_ssp_decoder->GetAPV();
        size_t N = decoded_data.size();
        TFile *f = new TFile("raw.root", "recreate");
        TH1I **h = new TH1I*[N];
        int nAPV = 0;
        for(auto &i: decoded_data) {
            int nBins = static_cast<int>(i.second.size());
            h[nAPV] = new TH1I(Form("h%d", nAPV), "raw", nBins, 0, nBins);

            int ts = 0;
            for(auto &j: i.second)
            {
                h[nAPV]->SetBinContent(ts, j);
                ts++;
            }

            nAPV++;
        }
        f->Write();
        std::cout<<"total apvs: "<<decoded_data.size()<<std::endl;
        std::cout<<"e to c..."<<std::endl;
        getchar();
    }

    std::cout<<"Total events read: "<<file_reader->GetEventNumber()
             <<std::endl;

    return 0;
}
