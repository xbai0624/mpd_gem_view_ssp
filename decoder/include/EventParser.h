#ifndef EVENT_PARSER_H
#define EVENT_PARSER_H

////////////////////////////////////////////////////////////////
// Class to parse event 
// It receives a buffer, and then parse that buffer into 
// event banks, then call the corresponding raw decoders

#include "GeneralEvioStruct.h"
#include "AbstractRawDecoder.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

class EventParser
{
public:
    EventParser();
    ~EventParser();

    void ParseEvent(const uint32_t *pBuf, uint32_t fBufLen);

    void ParseBank(const uint32_t *pBuf, uint32_t fBufLen);
    void ParseSegment(const uint32_t *pBuf, uint32_t fBufLen);
    void ParseTagSegment(const uint32_t *pBuf, uint32_t fBufLen);

    // vTagTrack saves hierarchy bank tag number
    void SeparateSubHierarchy(const uint32_t *pBuf, uint32_t fBufLen, 
            EvioPrimitiveDataType content_type, EvioPrimitiveDataType self_type,
            std::vector<int> &vTagTrack);

    void ParseData(const uint32_t *pBuf, uint32_t fBufLen, 
            EvioPrimitiveDataType content_type, EvioPrimitiveDataType self_type,
            std::vector<int> &vTagTrack);

    void RegisterRawDecoder(int, AbstractRawDecoder* decoder);
    AbstractRawDecoder* GetRawDecoder(int);

    void Reset();
    void ClearForNextEvent();

    void SetEventNumber(int);
    uint32_t GetEventNumber();

private:
    // {tag -> decoder}, decode data according to tag
    std::unordered_map<int, AbstractRawDecoder*> mDecoder;

    uint32_t event_number = 0;
};

#endif
