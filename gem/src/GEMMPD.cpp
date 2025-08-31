//============================================================================//
// GEM MPD class                                                              //
// MPD contains several APVs                                                  //
// The memory of included APV classes will be managed by MPD                  //
//                                                                            //
// Chao Peng     10/07/2016                                                   //
// Xinzhan Bai   12/01/2020                                                   // 
//============================================================================//

#include "GEMMPD.h"
#include "GEMSystem.h"
#include "GEMAPV.h"
#include <iostream>
#include <algorithm>


//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//


////////////////////////////////////////////////////////////////////////////////
// constructor

GEMMPD::GEMMPD(const int &cid,
               const int &mid,
               const std::string &p,
               const int &slots,
               GEMSystem *g)
: gem_sys(g), id(mid), crate_id(cid), ip(p)
{
    // open slots for inserting APVs
    adc_list.resize(slots, nullptr);

    addr.crate_id = cid;
    addr.mpd_id = mid;
}

////////////////////////////////////////////////////////////////////////////////
// copy constructor

GEMMPD::GEMMPD(const GEMMPD &that)
: gem_sys(nullptr), id(that.id), crate_id(that.crate_id), addr(that.addr), ip(that.ip)
{
    // open slots for inserting APVs
    adc_list.resize(that.adc_list.size(), nullptr);

    for(uint32_t i = 0; i < that.adc_list.size(); ++i)
    {
        // add the copied apv to the same slot
        if(that.adc_list.at(i) != nullptr)
            AddAPV(new GEMAPV(*that.adc_list.at(i)), i);
    }
}

////////////////////////////////////////////////////////////////////////////////
// move constructor

GEMMPD::GEMMPD(GEMMPD &&that)
: gem_sys(nullptr), id(that.id), crate_id(that.crate_id), addr(std::move(that.addr)), 
    ip(std::move(that.ip)), adc_list(std::move(that.adc_list))
{
    // reset the connection
    for(uint32_t i = 0; i < adc_list.size(); ++i)
        adc_list[i]->SetMPD(this, i);
}

////////////////////////////////////////////////////////////////////////////////
// desctructor

GEMMPD::~GEMMPD()
{
    UnsetSystem();

    Clear();
}

////////////////////////////////////////////////////////////////////////////////
// copy constructor

GEMMPD &GEMMPD::operator =(const GEMMPD &rhs)
{
    Clear();

    crate_id = rhs.crate_id;
    id = rhs.id;
    addr = rhs.addr;
    ip = rhs.ip;
    adc_list.resize(rhs.adc_list.size(), nullptr);

    for(uint32_t i = 0; i < rhs.adc_list.size(); ++i)
    {
        // add the copied apv to the same slot
        if(rhs.adc_list.at(i) != nullptr)
            AddAPV(new GEMAPV(*rhs.adc_list.at(i)), i);
    }
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// move constructor

GEMMPD &GEMMPD::operator =(GEMMPD &&rhs)
{
    Clear();

    crate_id = rhs.crate_id;
    id = rhs.id;
    addr = std::move(rhs.addr);
    ip = std::move(rhs.ip);
    adc_list = std::move(rhs.adc_list);

    // reset the connection
    for(uint32_t i = 0; i < adc_list.size(); ++i)
        adc_list[i]->SetMPD(this, i);
    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

////////////////////////////////////////////////////////////////////////////////
// set the GEM System for MPD, and disconnect it from the previous GEM System

void GEMMPD::SetSystem(GEMSystem *g, bool force_set)
{
    if(g == gem_sys)
        return;

    if(!force_set)
        UnsetSystem();

    gem_sys = g;
}

////////////////////////////////////////////////////////////////////////////////
// unset gem system

void GEMMPD::UnsetSystem(bool force_unset)
{
    if(!gem_sys)
        return;

    if(!force_unset)
        gem_sys->DisconnectMPD(addr, true);

    gem_sys = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// change the capacity

void GEMMPD::SetCapacity(int slots)
{
    // capacity cannot be negative
    if(slots < 0) slots = 0;

    if((uint32_t)slots < adc_list.size())
    {
        std::cout << " GEM MPD Warning: Reduce the slots in MPD "
                  << id << " from " << adc_list.size() << " to " << slots
                  << ". All APVs beyond " << slots << " will be removed. "
                  << std::endl;

        for(uint32_t i = slots; i < adc_list.size(); ++i)
        {
            if(adc_list[i] != nullptr)
                adc_list[i]->UnsetMPD(true);
        }
    }

    adc_list.resize(slots, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// set mpd address

void GEMMPD::SetAddress(const MPDAddress &ad)
{
    addr = ad;

    crate_id = addr.crate_id;
    id = addr.mpd_id;
}

////////////////////////////////////////////////////////////////////////////////
// add an apv to mpd, return false if failed

bool GEMMPD::AddAPV(GEMAPV *apv, const int &slot)
{
    if(apv == nullptr)
        return false;

    if((uint32_t)slot >= adc_list.size()) {
        std::cerr << "GEM MPD " << id
                  << ": Abort to add an apv to adc channel "
                  << apv->GetADCChannel()
                  << ", this MPD only has " << adc_list.size()
                  << " channels. (defined in GEMMPD.h)"
                  << std::endl;
        return false;
    }

    if(adc_list.at(slot) != nullptr) {
        std::cerr << "GEM MPD " << id
                  << ": Abort to add an apv to adc channel "
                  << apv->GetADCChannel()
                  << ", channel is occupied"
                  << std::endl;
        return false;
    }

    adc_list[slot] = apv;
    apv->SetMPD(this, slot);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// remove apv in the slot

void GEMMPD::RemoveAPV(const int &slot)
{
    if((uint32_t)slot >= adc_list.size())
        return;

    auto &apv = adc_list[slot];
    if(apv) {
        apv->UnsetMPD(true);
        delete apv, apv = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
// disconnect apv in the slot

void GEMMPD::DisconnectAPV(const int &slot, bool force_disconn)
{
    if((uint32_t)slot >= adc_list.size())
        return;

    auto &apv = adc_list[slot];
    if(!apv)
        return;

    if(!force_disconn)
        apv->UnsetMPD(true);

    apv = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// clear all the apvs in mpd

void GEMMPD::Clear()
{
    for(auto &adc : adc_list)
    {
        // prevent calling remove apv
        if(adc)
            adc->UnsetMPD(true);
        delete adc;
    }
}

////////////////////////////////////////////////////////////////////////////////
// get the apv in the slot

GEMAPV *GEMMPD::GetAPV(const int &slot)
const
{
    if((uint32_t)slot >= adc_list.size())
        return nullptr;

    return adc_list[slot];
}

////////////////////////////////////////////////////////////////////////////////
// get apv list in this mpd

std::vector<GEMAPV*> GEMMPD::GetAPVList()
const
{
    std::vector<GEMAPV*> result;
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            result.push_back(adc);
    }
    return result;
}

