#ifndef TRIGGER_DECODER_H
#define TRIGGER_DECODER_H

#include "AbstractRawDecoder.h"

class TriggerDecoder : public AbstractRawDecoder
{
    public:
        TriggerDecoder();
        ~TriggerDecoder();

        void Decode(const uint32_t *pBuf, uint32_t fBufLen,
                std::vector<int> &vTagTrack);

        std::pair<uint32_t, uint32_t> GetDecoded() const;

        void Clear();

    private:
        uint32_t trigger_time_l;
        uint32_t trigger_time_h;
};


#endif
