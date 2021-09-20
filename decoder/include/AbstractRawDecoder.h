#ifndef ABSTRACT_RAW_DECODER_H
#define ABSTRACT_RAW_DECODER_H

#include <cstdint>
#include <vector>
#include "MPDDataStruct.h"

////////////////////////////////////////////////////////////////
// An interface for registering all daughter detector raw decoders

class AbstractRawDecoder
{
public:
    // default constructor
    AbstractRawDecoder();
    // disable copy constructor
    AbstractRawDecoder(const AbstractRawDecoder &) = delete;
    // disable copy assignment
    AbstractRawDecoder & operator = (const AbstractRawDecoder &) = delete;
    // disable move constructor
    AbstractRawDecoder(AbstractRawDecoder &&) = delete;
    // disable move assignment
    AbstractRawDecoder & operator = (AbstractRawDecoder &&) = delete;

    virtual ~AbstractRawDecoder();

    // vTagTrack saves the track of Bank tags in its upper hierarchy banks
    virtual void Decode(const uint32_t *pBuf, uint32_t fBufLen, 
            std::vector<int> &vTagTrack) = 0;

    virtual void Clear() = 0;

private:
};

#endif
