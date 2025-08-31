#include "EPICSystem.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>

#define EPICS_UNDEFINED_VALUE -9999.9

EPICSystem::EPICSystem(const std::string &path)
{
    ReadMap(path);
}

EPICSystem::~EPICSystem()
{
    // place holder
}

void EPICSystem::Reset()
{
    epics_data = std::deque<EpicsData>();

    for(auto &value : epics_values)
    {
        value = EPICS_UNDEFINED_VALUE;
    }
}

static std::string strip(const std::string &s)
{
    std::string _t("");
    size_t start = s.find_first_not_of(" ");
    size_t end = s.find_last_not_of(" ");

    if(end < start) {
        return _t;
    }

    _t = s.substr(start, end-start+1);

    return _t;
}

static std::vector<std::string> __read_epics_map(const std::string &path)
{
    std::vector<std::string> res;

    if(path.empty()) {
        std::cout<<__PRETTY_FUNCTION__<<" ERROR: cannot find epics map:"
            <<path<<std::endl;
        return res;
    }

    std::fstream f_m(path.c_str(), std::fstream::in);
    std::string line;
    while(std::getline(f_m, line))
    {
        std::string tmp = strip(line);
        if(tmp[0] == '#') continue;

        res.push_back(tmp);
    }

    return res;
}

void EPICSystem::ReadMap(const std::string &path)
{
    if(path.empty())
        return;

    std::cout<<__PRETTY_FUNCTION__<<" INFO: Loading epics map from: "
        <<path<<std::endl;

    std::vector<std::string> epics_map = __read_epics_map(path);

    for(auto &i: epics_map)
    {
        AddChannel(i);
    }
}

void EPICSystem::AddChannel(const std::string &name)
{
    auto it = epics_map.find(name);

    if(it == epics_map.end()) {
        epics_map[name] = epics_values.size();
        epics_values.push_back(EPICS_UNDEFINED_VALUE);
    } else {
        std::cout << " EPICS Warning: Failed to add duplicated channel "
                  << name << ", its channel id is " << it->second
                  << std::endl;
    }
}

void EPICSystem::AddChannel(const std::string &name, uint32_t id, float value)
{
    if(id >= (uint32_t)epics_values.size())
    {
        epics_values.resize(id + 1, EPICS_UNDEFINED_VALUE);
    }

    epics_map[name] = id;
    epics_values.at(id) = value;
}

void EPICSystem::UpdateChannel(const std::string &name, const float &value)
{
    auto it = epics_map.find(name);
    if(it != epics_map.end()) {
        epics_values[it->second] = value;
    }
}

void EPICSystem::FillRawData(const char *data)
{
    std::string raw_data(data);

    std::istringstream iss(raw_data);
    std::string tmp;
    std::vector<std::string> cache;
    while(iss >> tmp) {
        std::string _t = strip(tmp);

        if(_t.size() > 0) cache.push_back(_t);
    }

    int N = cache.size();

    // expect 2 elements for each epics channel
    // channel_name  channel_value
    assert(N % 2 == 0);

    std::string name;
    float number;
    for(int i=0; i<N/2; i++)
    {
        name = cache[2*i];
        try {
            number = std::stod(cache[2*i+1]);
        } catch (...) {
            std::cout<<__PRETTY_FUNCTION__<<" ERROR: failed to convert epics channel value:"
                <<cache[2*i + 1]<<std::endl;
        }

        UpdateChannel(name, number);
    }
}

void EPICSystem::AddEvent(EpicsData &&data)
{
    epics_data.emplace_back(data);
}

void EPICSystem::AddEvent(const EpicsData &data)
{
    epics_data.push_back(data);
}

void EPICSystem::SaveData(const int &event_number, bool online)
{
    if(online && epics_data.size())
        epics_data.pop_front();

    epics_data.emplace_back(event_number, epics_values);
}

float EPICSystem::GetValue(const std::string &name)
const
{
    unsigned int ch = GetChannel(name);

    if(epics_data.size() && epics_data.back().values.size() > ch)
        return epics_data.back().values.at(ch);

    return EPICS_UNDEFINED_VALUE;
}

int EPICSystem::GetEventNumber()
const
{
    if(epics_data.empty())
        return -1;

    return epics_data.back().event_number;
}

int EPICSystem::GetChannel(const std::string &name)
const
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        std::cout << " EPICS Warning: Did not find EPICS channel "
                  << name
                  << std::endl;
        return -1;
    }

    return it->second;
}

std::vector<EPICSChannel> EPICSystem::GetSortedList()
const
{
    std::vector<EPICSChannel> epics_list;

    for(auto &ch : epics_map)
    {
        float value = epics_values.at(ch.second);
        epics_list.emplace_back(ch.first, ch.second, value);
    }

    std::sort(epics_list.begin(), epics_list.end(),
              [] (const EPICSChannel &a, const EPICSChannel &b)
              {return a.id < b.id;});

    return epics_list;
}

void EPICSystem::SaveMap(const std::string &path)
const
{
    std::ofstream out(path);

    if(!out.is_open()) {
        std::cerr << " EPICS Error: Cannot open file "
                  << "\"" << path << "\""
                  << " to save EPICS channels!"
                  << std::endl;
        return;
    }

    std::vector<EPICSChannel> epics_list = GetSortedList();

    for(auto &ch : epics_list)
    {
        out << ch.name << std::endl;
    }

    out.close();
}

const EpicsData &EPICSystem::GetEvent(const unsigned int &index)
const
{
    if(epics_data.empty()) {
        std::cout<<__PRETTY_FUNCTION__<<" ERROR: emtpy data bank!"
            <<std::endl;
    }

    if(index >= epics_data.size())
        return epics_data.back();

    return epics_data.at(index);
}

float EPICSystem::FindValue(int evt, const std::string &name)
const
{
    float result = EPICS_UNDEFINED_VALUE;

    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        std::cerr << " EPICS Warning: Did not find EPICS channel "
                  << name
                  << std::endl;
        return EPICS_UNDEFINED_VALUE;
    }

    uint32_t channel_id = it->second;

    auto ev_it = binary_search_close_less(epics_data.begin(), epics_data.end(), evt);

    // found the epics event that just before evt, and it has that channel
    if((ev_it != epics_data.end()) &&
       (ev_it->values.size() > channel_id)) {
        result = ev_it->values.at(channel_id);
    }

    return result;
}

int EPICSystem::FindEvent(int evt)
const
{
    auto it = binary_search_close_less(epics_data.begin(), epics_data.end(), evt);

    // found the epics event that just before evt, and it has that channel
    if(it != epics_data.end())
        return (it - epics_data.begin());

    return -1;
}
