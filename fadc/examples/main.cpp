#include <iostream>
#include <fstream>

#include "EvioFileReader.h"
#include "EventParser.h"
#include "RolStruct.h"

#include "Fadc250Decoder.h"
#include "WfRootGraph.h"

#include <TFile.h>

using namespace fdec;
using namespace std;

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[])
{
    // set up the evio file reader
    EvioFileReader *file_reader = new EvioFileReader();
    //file_reader -> SetFile("/home/daq/test/fadc250/SoLIDCer500.dat.0");
    file_reader -> SetFile("/home/daq/test/fadc250/SoLIDCer516.dat.0");
    file_reader -> OpenFile();

    // set up the event parser
    EventParser *event_parser = new EventParser();

    // initialize all detector raw decoders
    Fadc250Decoder *fadc_decoder = new Fadc250Decoder();

    // register all detector raw decoders
    event_parser->RegisterRawDecoder(static_cast<int>(Bank_TagID::FADC), fadc_decoder);

    // reserve a fadc250 analyzer
    Analyzer waveform_ana;

    // read event to buffer
    const uint32_t *pBuf;
    uint32_t fBufLen;
    while(file_reader->ReadNoCopy(&pBuf, &fBufLen) == S_SUCCESS)
    {
        event_parser->ParseEvent(pBuf, fBufLen);

        const Fadc250Event &event = fadc_decoder -> GetDecodedEvent();

        int nch = 0;
        // analyze channel data
        for(auto &i: event.channels)
        {
            Fadc250Data data = i;
            waveform_ana.Analyze(data);
            WfRootGraph g = get_waveform_graph(waveform_ana, data.raw);
            cout<<"sample points: "<<data.raw.size()<<endl;

            // save raw adc for each channel
            fstream f_txt(Form("plots/raw_adc_channel%d.txt", nch), fstream::out);
            for(auto &adc_raw: i.raw) {
                f_txt<<adc_raw<<endl;
            }
            f_txt.close();

            // save plots to root
            cout<<"writing.. channel: "<<nch<<endl;
            TFile *f = new TFile(Form("plots/res_channel%d.root", nch), "recreate");
            for(auto &ee: g.entries)
            {
                (TGraph*)ee.obj -> Write(ee.label.c_str());
            }
            g.mg->Write(Form("mg_ch%d", nch));
            f->Close();
            nch++;
        }
        getchar();

    }

    return 0;
}
