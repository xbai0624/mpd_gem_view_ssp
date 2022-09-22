#ifndef __EPIC_SYSTEM_H
#define __EPIC_SYSTEM_H

#include <vector>
#include <string>
#include <deque>
#include <unordered_map>

// epics channel
struct EPICSChannel
{
    std::string name;
    uint32_t id;
    float value;

    EPICSChannel(const std::string &n, const uint32_t &i, const float &v)
        : name(n), id(i), value(v)
    {}
};

//============================================================================//
// *BEGIN* RAW EPICS DATA STRUCTURE                                           //
//============================================================================//
struct EpicsData
{
    int32_t event_number;
    std::vector<float> values;

    EpicsData()
    {}
    EpicsData(const int &ev, const std::vector<float> &val)
        : event_number(ev), values(val)
    {}

    void clear()
    {
        event_number = 0;
        values.clear();
    }

    bool operator <(const int &evt) const {return event_number < evt;}
    bool operator >(const int &evt) const {return event_number > evt;}
    bool operator <=(const int &evt) const {return event_number <= evt;}
    bool operator >=(const int &evt) const {return event_number >= evt;}
    bool operator ==(const int &evt) const {return event_number == evt;}
    bool operator !=(const int &evt) const {return event_number != evt;}
};
//============================================================================//
// *END* RAW EPICS DATA STRUCTURE                                             //
//============================================================================//


class EPICSystem
{
public:
    EPICSystem(const std::string &s);
    ~EPICSystem();

    void Reset();
    void ReadMap(const std::string &path);
    void SaveMap(const std::string &path) const;
    void AddChannel(const std::string &name);
    void AddChannel(const std::string &name, uint32_t id, float value);
    void UpdateChannel(const std::string &name, const float &value);
    void AddEvent(EpicsData &&data);
    void AddEvent(const EpicsData &data);
    void FillRawData(const char *buf);
    void SaveData(const int &event_number, bool online = false);

    std::vector<EPICSChannel> GetSortedList() const;
    const std::vector<float> &GetCurrentValues() const {return epics_values;}
    float GetValue(const std::string &name) const;
    int GetEventNumber() const;
    int GetChannel(const std::string &name) const;
    const EpicsData &GetEvent(const unsigned int &index) const;
    const std::deque<EpicsData> &GetEventData() const {return epics_data;}
    unsigned int GetEventCount() const {return epics_data.size();}
    float FindValue(int event_number, const std::string &name) const;
    int FindEvent(int event_number) const;

    // binary search, return the closest smaller value of the input if the same
    // value is not found
    template<class RdmaccIt, typename T>
    RdmaccIt binary_search_close_less(RdmaccIt beg, RdmaccIt end, const T &val) const
    {
        if(beg == end)
            return end;
        if(*(end - 1) <= val)
            return end - 1;

        RdmaccIt first = beg, last = end;
        RdmaccIt mid = beg + (end - beg)/2;
        while(mid != end)
        {
            if(*mid == val)
                return mid;
            if(*mid > val)
                end = mid;
            else
                beg = mid + 1;

            mid = beg + (end - beg)/2;
        }

        if(*mid < val) {
            return mid;
        } else {
            if(mid == first)
                return last;
            return mid - 1;
        }
    }


private:
    // data related
    std::unordered_map<std::string, uint32_t> epics_map;
    std::vector<float> epics_values;
    std::deque<EpicsData> epics_data;
};

#endif
