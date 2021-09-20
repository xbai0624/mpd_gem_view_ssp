#ifndef GEM_MPD_H
#define GEM_MPD_H

#include <vector>
#include "GEMAPV.h"

// maximum channels in a MPD
#define MPD_CAPACITY 16

class GEMSystem;

class GEMMPD
{
public:
    // constructor
    GEMMPD(const int &crate_id,
           const int &mpd_id,
           const std::string &ip,
           const int &slots = MPD_CAPACITY,
           GEMSystem *g = nullptr);

    // copy/move constructors
    GEMMPD(const GEMMPD &that);
    GEMMPD(GEMMPD &&that);

    // descructor
    virtual ~GEMMPD();

    // copy/move assignment operators
    GEMMPD &operator =(const GEMMPD &rhs);
    GEMMPD &operator =(GEMMPD &&rhs);

    // public member functions
    void SetSystem(GEMSystem *g, bool force_set = false);
    void UnsetSystem(bool force_unset = false);
    void SetCapacity(int slots);
    void SetAddress(const MPDAddress &ad);
    void SetCrateID(const int &i){crate_id = i;}
    void SetMPDID(const int &i){id = i;}
    bool AddAPV(GEMAPV *apv, const int &slot);
    void RemoveAPV(const int &slot);
    void DisconnectAPV(const int &slot, bool force_disconn = false);
    void Clear();

    // get parameters
    GEMSystem *GetSystem() const {return gem_sys;}
    int GetID() const {return id;}
    int GetCrateID() const {return crate_id;}
    const MPDAddress &GetAddress() const {return addr;}
    MPDAddress &GetAddress() {return addr;}
    const std::string &GetIP() const {return ip;}
    uint32_t GetCapacity() const {return adc_list.size();}
    GEMAPV *GetAPV(const int &slot) const;
    std::vector<GEMAPV*> GetAPVList() const;

    // functions apply to all apv members
    template<typename... Args>
        void APVControl(void (GEMAPV::*act)(Args...), Args&&... args)
        {
            for(auto apv : adc_list)
            {
                if(apv != nullptr)
                    (apv->*act)(std::forward<Args>(args)...);
            }
        }

private:
    GEMSystem *gem_sys;
    int id;
    int crate_id;
    MPDAddress addr;
    std::string ip;
    std::vector<GEMAPV*> adc_list;
};

#endif
