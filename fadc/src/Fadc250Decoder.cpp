//
// A simple decoder to process the JLab FADC250 data
// Reference: https://www.jlab.org/Hall-B/ftof/manuals/FADC250UsersManual.pdf
//
// Author: Chao Peng
// Date: 2020/08/22
//

#include "Fadc250Decoder.h"

using namespace fdec;

#define SET_BIT(n,i)  ( (n) |= (1ULL << i) )
#define TEST_BIT(n,i)  ( (bool)( n & (1ULL << i) ) )

inline void print_word(uint32_t word)
{
    std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << word << std::dec << "\n";
}

inline Fadc250Data &get_channel(Fadc250Event &ev, uint32_t ch)
{
    return ev.channels[ch];
}

template<class Container>
inline uint32_t fill_in_words(const uint32_t *buf, size_t beg, Container &raw_data, size_t max_words = -1)
{
    uint32_t nwords = 0;
    for (uint32_t i = beg + 1; raw_data.size() < max_words; ++i, ++nwords) {
        auto data = buf[i];
        // finished
        if ((data & 0x80000000) && nwords > 0) {
            return nwords;
        }

        if (!(data & 0x20000000)) {
            raw_data.push_back((data >> 16) & 0x1FFF);
        }
        if (!(data & 0x2000)) {
            raw_data.push_back((data & 0x1FFF));
        }
    }
    return nwords;
}


Fadc250Decoder::Fadc250Decoder(double clk)
: _clk(clk)
{
    // place holder
}

// a help structure to save peak infos
struct PeakBuffer {
    uint32_t height = 0., integral = 0., time = 0.;
    bool in_data = false;
};

void Fadc250Decoder::DecodeEvent(Fadc250Event &res, const uint32_t *buf, size_t buflen)
const
{
    res.Clear();

    // sanity check
    if (!buflen) {
        return;
    }

    auto header = buf[0];

    // event header
    if (!(header & 0x80000000) || ((header >> 27) & 0xF) != EventHeader) {
        std::cout << "Fadc250Decoder Error: incorrect event header:";
        print_word(buf[0]);
        return;
    }

    res.number = (header & 0x3FFFFF);
    std::vector<std::vector<PeakBuffer>> peak_buffers(res.channels.size());
    uint32_t type = FillerWord;

    for (size_t iw = 1; iw < buflen; ++iw) {
        uint32_t data = buf[iw];

        // new type word, update the current type
        bool new_type = (data & 0x80000000);
        if (new_type) {
            type = (data >> 27) & 0xF;
            SET_BIT(res.mode, type);
        }

        switch (type) {
        // trigger timing, might be multiple timing words
        case TriggerTime:
            res.time.push_back(data & 0xFFFFFF);
            break;
        // window raw data
        case WindowRawData:
            if (new_type) {
                // get channel and window size
                uint32_t ch = (data >> 23) & 0xF;
                size_t nwords= (data & 0xFFF);
                auto &raw_data = get_channel(res, ch).raw;
                raw_data.clear();
                iw += fill_in_words(buf, iw, raw_data, nwords);
            } else {
                std::cout << "Fadc250Decoder Error: unexpected window raw data word. ";
                print_word(data);
            }
            break;
        // pulse raw data, TODO: currently unsupported
        case PulseRawData:
            std::cout << "Fadc250Decoder Warning: unsupported data mode: pulse raw data, skip it" << std::endl;
            /*
            {
                uint32_t ch = (data >> 23) & 0xF;
                uint32_t pulse_num = (data >> 21) & 0x3;
                uint32_t first_sample = data & 0x3FF;
            }
            */
            break;
        // pulse integral
        case PulseIntegral:
            {
                uint32_t ch = (data >> 23) & 0xF;
                uint32_t pulse_num = (data >> 21) & 0x3;
                // uint32_t quality = (data >> 19) & 0x3;
                if (peak_buffers[ch].size() < pulse_num + 1) {
                    peak_buffers[ch].resize(4);
                }
                peak_buffers[ch][pulse_num].integral = data & 0x7FFFF;
                peak_buffers[ch][pulse_num].in_data = true;
            }
            break;
        case PulseTime:
            {
                uint32_t ch = (data >> 23) & 0xF;
                uint32_t pulse_num = (data >> 21) & 0x3;
                // uint32_t quality = (data >> 19) & 0x3;
                [[maybe_unused]]auto &chan = get_channel(res, ch);
                if (peak_buffers[ch].size() < pulse_num + 1) {
                    peak_buffers[ch].resize(4);
                }
                // convert to ns (1e3 / _clk (MHz) / 64)
                peak_buffers[ch][pulse_num].time = data & 0xFFFF;
                peak_buffers[ch][pulse_num].in_data = true;
            }
            break;
        case Scaler:
            // TODO
            // std::cout << "Scaler word: nwords = " << (data & 0x3F) << std::endl;
            break;
        case InvalidData:
        case FillerWord:
            break;
        case BlockHeader:
            break;
        case BlockTrailer:
            break;
        default:
            std::cout << "Error: unexpected data type " << type << " in header processing. ";
            print_word(data);
            return;
        }
    }

    // fill peak buffers to result
    for (size_t i = 0; i < peak_buffers.size(); ++i) {
        for (auto &peak : peak_buffers[i]) {
            if (!peak.in_data) {
                continue;
            }
            // time conversion: 1000/(clk/MHz*64) ns
            res.channels[i].peaks.emplace_back(static_cast<double>(peak.height),
                                               static_cast<double>(peak.integral),
                                               static_cast<double>(peak.time)*15.625/_clk);
        }
    }

    return;
}

void Fadc250Decoder::Decode([[maybe_unused]]const uint32_t *pBuf, 
        [[maybe_unused]]uint32_t fBufLen, 
        [[maybe_unused]]std::vector<int> &vTagTrack)
{
    // skip block header information
    Fadc250Event event = DecodeEvent(&pBuf[2], fBufLen);
    _event = std::move(event);
}

void Fadc250Decoder::Clear()
{
}
