#ifndef GENERAL_EVIO_STRUCT_H_
#define GENERAL_EVIO_STRUCT_H_

////////////////////////////////////////////////////////////////
// A file defines the coda event struct in raw data files (evio)

#include <unordered_map>

////////////////////////////////////////////////////////////////
// event bank header

struct EventBankHeader 
{
    EventBankHeader() : length(0), tag(0), pad(0), type(0), num(0), status(0)
    {}

    EventBankHeader(uint32_t word1, uint32_t word2)
    {
        length = static_cast<int>(word1);

        num  = word2       & 0xff;
        type = (word2>>8)  & 0x3f;
        pad  = (word2>>14) & 0x3;
        //tag  = (word2>>16) & 0xffff; // old version
        tag  = (word2>>16) & 0x0fff; // new version
        status = (word2>>28) & 0xf; // new version
    }

    int length;    // event buffer length 
    int tag;       // event tag
    int pad;       // event pad
    int type;      // event type
    int num;       // num
    int status;    // status, this is from Ben, no info found in EVIO manual
};

////////////////////////////////////////////////////////////////
// event segment header

struct EventSegmentHeader
{
    EventSegmentHeader(): tag(0), pad(0), type(0), length(0)
    {}

    EventSegmentHeader(uint32_t word)
    {
        length = word       & 0xffff;
        type   = (word>>16) & 0x3f;
        pad    = (word>>22) & 0x3;
        tag    = (word>>24) & 0xff;
    }

    int tag;
    int pad;
    int type;
    int length;
};

////////////////////////////////////////////////////////////////
// event tagsegment header

struct EventTagSegmentHeader
{
    EventTagSegmentHeader(): tag(0), type(0), length(0)
    {}

    EventTagSegmentHeader(uint32_t word)
    {
        length = word & 0xffff;
        type   = (word>>16) & 0xf;
        tag    = (word>>20) & 0xfff;
    }

    int tag;
    int type;
    int length;
};

////////////////////////////////////////////////////////////////
// evio bank primitive data type
// copied from evio user guide

enum class EvioPrimitiveDataType
{
    Bank,
    Segment,
    TagSegment,
    Composite,
    Unknown32Bit,
    SignedInt32Bit,
    UnsignedInt32Bit,
    Float32Bit,
    SignedChar8Bit,
    UnsignedChar8Bit,
    Char8Bit,
    SignedShort16Bit,
    UnsignedShort16Bit,
    SignedInt64Bit,
    UnsignedInt64Bit,
    Double64Bit,
    Hollerit,
    N_Value,
    Undefined
};

////////////////////////////////////////////////////////////////
// a map for looking up evio primitive data type

const std::unordered_map<int, EvioPrimitiveDataType> mapEvioPrimitiveDataType = 
{
    {0x0,  EvioPrimitiveDataType::Unknown32Bit},
    {0x1,  EvioPrimitiveDataType::UnsignedInt32Bit},
    {0x2,  EvioPrimitiveDataType::Float32Bit},
    {0x3,  EvioPrimitiveDataType::Char8Bit},
    {0x4,  EvioPrimitiveDataType::SignedShort16Bit},
    {0x5,  EvioPrimitiveDataType::UnsignedShort16Bit},
    {0x6,  EvioPrimitiveDataType::SignedChar8Bit},
    {0x7,  EvioPrimitiveDataType::UnsignedChar8Bit},
    {0x8,  EvioPrimitiveDataType::Double64Bit},
    {0x9,  EvioPrimitiveDataType::SignedInt64Bit},
    {0xa,  EvioPrimitiveDataType::UnsignedInt64Bit},
    {0xb,  EvioPrimitiveDataType::SignedInt32Bit},
    {0xc,  EvioPrimitiveDataType::TagSegment},
    {0xd,  EvioPrimitiveDataType::Segment},
    {0xe,  EvioPrimitiveDataType::Bank},
    {0xf,  EvioPrimitiveDataType::Composite},
    {0x10, EvioPrimitiveDataType::Bank},
    {0x20, EvioPrimitiveDataType::Segment},
    {0x21, EvioPrimitiveDataType::Hollerit},
    {0x22, EvioPrimitiveDataType::N_Value},
};

////////////////////////////////////////////////////////////////
// a wrapper for looking up the primitive data type map

inline EvioPrimitiveDataType DataType(int key) 
{
    if(mapEvioPrimitiveDataType.find(key) != mapEvioPrimitiveDataType.end())
        return mapEvioPrimitiveDataType.at(key);

    return EvioPrimitiveDataType::Undefined;
};

#endif
