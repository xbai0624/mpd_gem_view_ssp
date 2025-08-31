#include "EventParser.h"

#include <iostream>

////////////////////////////////////////////////////////////////
// ctor

EventParser::EventParser()
{
}

////////////////////////////////////////////////////////////////
// dtor

EventParser::~EventParser()
{
}

////////////////////////////////////////////////////////////////
// parse event
//
// evio events are organized in a hierarchy structure
// Containter Banks contain other banks, leaf banks 
// contain an array of a single primitive data
//
// this function first test the type of the buffer it received,
// then calls the corresponding process routines

void EventParser::ParseEvent(const uint32_t *pBuf, uint32_t fBufLen)
{
    // clear last event
    ClearForNextEvent();

    // found a correct event type using bank header, which means
    // that this buffer is a bank
    EventBankHeader event_header(pBuf[0], pBuf[1]);
    if(DataType(event_header.type) != EvioPrimitiveDataType::Undefined) 
    {
        ParseBank(pBuf, fBufLen);
        event_number++;
        return;
    }

    // according to evio manual, the first bank in a buffer or event must
    // be a BANK, so the following two sections should never be reached

    // found a correct event type using segment header
    EventSegmentHeader event_header_seg(pBuf[0]);
    if(DataType(event_header_seg.type) != EvioPrimitiveDataType::Undefined) 
    {
        ParseSegment(pBuf, fBufLen);
        event_number++;
        return;
    }

    // found a correct eventtype using TagSegment header
    EventTagSegmentHeader event_header_tag_seg(pBuf[0]);
    if(DataType(event_header_tag_seg.type) != EvioPrimitiveDataType::Undefined) 
    {
        ParseTagSegment(pBuf, fBufLen);
        event_number++;
        return;
    }

    std::cout<<"Error: Unsupported bank type."<<std::endl;
}

////////////////////////////////////////////////////////////////
// parse event banks
// evio events are organized in a hierarchy structure
// event bank can contain Banks/Segments/TagSegments
// this function to process sub-level banks

void EventParser::ParseBank(const uint32_t *pBuf, uint32_t fBufLen)
{
    EventBankHeader event_header(pBuf[0], pBuf[1]);
    auto type = event_header.type;
    std::vector<int> vTagTrack(1, event_header.tag);

    SeparateSubHierarchy(pBuf, fBufLen, DataType(type), EvioPrimitiveDataType::Bank, vTagTrack);
}

////////////////////////////////////////////////////////////////
// parse event Segments 
// this function to process sub-level Segments

void EventParser::ParseSegment(const uint32_t *pBuf, uint32_t fBufLen)
{
    EventSegmentHeader event_header(pBuf[0]);
    auto type = event_header.type;
    std::vector<int> vTagTrack(1, event_header.tag);

    SeparateSubHierarchy(pBuf, fBufLen, DataType(type), EvioPrimitiveDataType::Segment, vTagTrack);
}

////////////////////////////////////////////////////////////////
// parse event TagSegments
// this function to process sub-level TagSegments

void EventParser::ParseTagSegment(const uint32_t *pBuf, uint32_t fBufLen)
{
    EventTagSegmentHeader event_header(pBuf[0]);
    auto type = event_header.type;
    std::vector<int> vTagTrack(1, event_header.tag);

    SeparateSubHierarchy(pBuf, fBufLen, DataType(type), EvioPrimitiveDataType::TagSegment, vTagTrack);
}

////////////////////////////////////////////////////////////////
// separate sub hierarchy structure
// if we encountered a container bank structure, this function 
// will separate all the sub banks out, and pass the separate banks
// to corresponding routines

void EventParser::SeparateSubHierarchy(const uint32_t *pBuf, uint32_t fBufLen, 
        EvioPrimitiveDataType content_type, EvioPrimitiveDataType self_type,
        std::vector<int> &vTagTrack)
{
    // content_type = type of data stored in this buffer "pBuf"
    // self_type = type of this buffer (this buffer itself is a BANK/Seg/TagSeg?)
    uint32_t pos = 0;

    // get header length
    if(self_type == EvioPrimitiveDataType::Bank) 
        pos = 2;
    else if(self_type == EvioPrimitiveDataType::Segment ||
            self_type == EvioPrimitiveDataType::TagSegment)
        pos = 1;

    switch(content_type) 
    {
        case EvioPrimitiveDataType::Bank:
            // the whole buffer is a hierarchy of banks
            while(pos < fBufLen) {
                EventBankHeader sub_bank_header(pBuf[pos], pBuf[pos+1]);
                vTagTrack.push_back(sub_bank_header.tag);
                // length from header does not include the length word itself
                // thus the total length should be (header.length+1)
                SeparateSubHierarchy(&pBuf[pos], sub_bank_header.length+1, 
                        DataType(sub_bank_header.type), content_type, vTagTrack);
                pos += (sub_bank_header.length+1);
                vTagTrack.pop_back();
            }
            break;
        case EvioPrimitiveDataType::Segment:
            // the whole buffer is a hierachy of segments
            while(pos < fBufLen){
                EventSegmentHeader sub_segment_header(pBuf[pos]);
                vTagTrack.push_back(sub_segment_header.tag);
                SeparateSubHierarchy(&pBuf[pos], sub_segment_header.length+1, 
                        DataType(sub_segment_header.type), content_type, vTagTrack);
                pos += (sub_segment_header.length+1);
                vTagTrack.pop_back();
            }
            break;
        case EvioPrimitiveDataType::TagSegment:
            // the whole buffer is a hierachy of tagsegments
            while(pos < fBufLen) {
                EventTagSegmentHeader sub_tagseg_header(pBuf[pos]);
                vTagTrack.push_back(sub_tagseg_header.tag);
                SeparateSubHierarchy(&pBuf[pos], sub_tagseg_header.length+1, 
                        DataType(sub_tagseg_header.type), content_type, vTagTrack);
                pos += (sub_tagseg_header.length+1);
                vTagTrack.pop_back();
            }
            break;
        case EvioPrimitiveDataType::Undefined:
            // unsupported evio data type
            break;
        default:
            {
                // the whole buffer is one data bank
                ParseData(pBuf, fBufLen, content_type, self_type, vTagTrack);
            }
    };
}

////////////////////////////////////////////////////////////////
// parse event data
// this function to process raw detector data in Banks/Segments/TagSegments

void EventParser::ParseData(const uint32_t *pBuf, [[maybe_unused]] uint32_t fBufLen, 
        [[maybe_unused]] EvioPrimitiveDataType content_type, EvioPrimitiveDataType self_type, 
        std::vector<int> &vTagTrack)
{
    // get tag
    [[maybe_unused]] int tag = 0, length = 0, header_length = 0;

    if(self_type == EvioPrimitiveDataType::Bank) {
        EventBankHeader header(pBuf[0], pBuf[1]);
        tag = header.tag; length = header.length;
        header_length = 2;
    }
    else if(self_type == EvioPrimitiveDataType::Segment) {
        EventSegmentHeader header(pBuf[0]);
        tag = header.tag; length = header.length;
        header_length = 1;
    }
    else if(self_type == EvioPrimitiveDataType::TagSegment) {
        EventTagSegmentHeader header(pBuf[0]);
        tag = header.tag; length = header.length;
        header_length = 1;
    }
    else {
        std::cout<<"Warning: Unsupported bank type."<<std::endl;
    }

    // decode
    if(mDecoder.find(tag) != mDecoder.end()) {
        mDecoder[tag] -> Decode(&pBuf[header_length], length-1, vTagTrack);
    }
}

////////////////////////////////////////////////////////////////
// register raw decoder

void EventParser::RegisterRawDecoder(int tag, AbstractRawDecoder* decoder)
{
    if(mDecoder.find(tag) != mDecoder.end()) {
        std::cout<<__PRETTY_FUNCTION__<<" Warning: decoder already registered: \" tag: "<<tag<<"\"."
                 <<std::endl;
        return;
    }

    mDecoder[tag] = decoder;
}

////////////////////////////////////////////////////////////////
// get raw decoder registered in this event parser according to 
// the bank tag

AbstractRawDecoder * EventParser::GetRawDecoder(int tag)
{
    if(mDecoder.find(tag) == mDecoder.end())
        return nullptr;
    return mDecoder[tag];
}

////////////////////////////////////////////////////////////////
// set event number

void EventParser::SetEventNumber(int n)
{
    event_number = static_cast<uint32_t>(n);
}


////////////////////////////////////////////////////////////////
// get event number

uint32_t EventParser::GetEventNumber()
{
    return event_number;
}

////////////////////////////////////////////////////////////////
// reset event parser

void EventParser::Reset()
{
    // reset event number
    event_number = 0;

    // clear all decoders
    for(auto &i: mDecoder)
        i.second->Clear();
}

////////////////////////////////////////////////////////////////
// clear all decoder for next event

void EventParser::ClearForNextEvent()
{
    // clear all decoders
    for(auto &i: mDecoder)
        i.second->Clear();
}
