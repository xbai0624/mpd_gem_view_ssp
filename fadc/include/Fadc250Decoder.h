#pragma once

//
// A simple decoder to process the JLab FADC250 data
// Reference: https://www.jlab.org/Hall-B/ftof/manuals/FADC250UsersManual.pdf
//
// Author: Chao Peng
// Date: 2020/08/22
//

#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include "Fadc250Data.h"
#include "AbstractRawDecoder.h"

namespace fdec
{

// data type
enum Fadc250Type {
    BlockHeader = 0,
    BlockTrailer = 1,
    EventHeader = 2,
    TriggerTime = 3,
    WindowRawData = 4,
    // 5 reserved
    PulseRawData = 6,
    PulseIntegral = 7,
    PulseTime = 8,
    // 9 - 11 reserved
    Scaler = 12,
    // 13 reserved
    InvalidData = 14,
    FillerWord = 15,
};

class Fadc250Event
{
public:
    uint32_t number, mode;
    std::vector<uint32_t> time;
    std::vector<Fadc250Data> channels;

    Fadc250Event(uint32_t n = 0, uint32_t nch = 16)
        : number(n), mode(0)
    {
        channels.resize(nch);
    }

    void Clear()
    {
        mode = 0;
        time.clear();
        for (auto &ch : channels) { ch.Clear(); }
    }
};

class Fadc250Decoder : public AbstractRawDecoder
{
public:
    Fadc250Decoder(double clk = 250.);

    // for an event data
    void DecodeEvent(Fadc250Event &event, const uint32_t *buf, size_t len) const;
    inline Fadc250Event DecodeEvent(const uint32_t *buf, size_t len, size_t nchans = 16) const
    {
        Fadc250Event evt;
        evt.channels.resize(nchans);
        DecodeEvent(evt, buf, len);
        return evt;
    }

    virtual void Decode(const uint32_t *pBuf, uint32_t fBufLen, std::vector<int> &vTagTrack);
    virtual void Clear();
    const Fadc250Event &GetDecodedEvent() const {return _event;}


private:
    double _clk;

    Fadc250Event _event;
};

}; // namespace fdec
