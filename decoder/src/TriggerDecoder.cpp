#include "TriggerDecoder.h"
#include "sspApvdec.h"
#include <iostream>

TriggerDecoder::TriggerDecoder()
    : trigger_time_l(0), trigger_time_h(0)
{
}

TriggerDecoder::~TriggerDecoder()
{
}

void TriggerDecoder::Clear()
{
    trigger_time_l = 0;
    trigger_time_h = 0;
}

std::pair<uint32_t, uint32_t> TriggerDecoder::GetDecoded()
const
{
    return std::pair<uint32_t, uint32_t>(trigger_time_l, trigger_time_h);
}

void TriggerDecoder::Decode(const uint32_t *pBuf, uint32_t fBufLen,
        std::vector<int> &vTagTrack)
{
    //for(auto &i: vTagTrack)
    //    std::cout<<"tirgger: "<<i<<std::endl;

    if(vTagTrack[1] != 65313)
    {
        //std::cout<<"non trigger bank..."<<std::endl;
        return;
    }
    //std::cout<<"-----------------"<<std::endl;

    if(fBufLen != 2) {
        std::cout<<"TriggerDecoder Error:: expecting 2 uin32_t words."
            <<std::endl
            <<"            received: "<<fBufLen<<" uint32_t words."
            <<std::endl;
    }

    sspApv_trigger_time_1_t word1;
    sspApv_trigger_time_2_t word2;

    word1.raw = pBuf[0];
    word2.raw = pBuf[1];

    trigger_time_l = word1.bf.trigger_time_l;
    trigger_time_h = word2.bf.trigger_time_h;

    //std::cout<<"data type tag:"<<word1.bf.data_type_tag<<", trigger time low: "<<word1.bf.trigger_time_l<<std::endl;
    //std::cout<<"data type tag:"<<word2.bf.data_type_tag<<", trigger time high: "<<word2.bf.trigger_time_h<<std::endl;
}
