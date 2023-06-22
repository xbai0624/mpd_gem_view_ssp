#include "Cuts.h"
#include "GEMStruct.h"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using std::fstream;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::istringstream;
using std::setfill;
using std::setw;

////////////////////////////////////////////////////////////////////////////////
//                                 members                                    //
////////////////////////////////////////////////////////////////////////////////

Cuts::~Cuts()
{
}

void Cuts::SetFile(const char* _path)
{
    path = _path;
}

void Cuts::Init()
{
    txt_parser.Configure("config/gem.conf");
    std::string path = txt_parser.Value<std::string>("GEM Tracking Config");

    std::cout<<"loading tracking config from : "<<path<<std::endl;
 
    SetFile(path.c_str());
    LoadFile();

    __convert_map();
}

void Cuts::LoadFile()
{
    fstream input_file(path.c_str(), fstream::in);
    if(!input_file.is_open()) {
        cout<<"Warning:: couldn't load config file for cuts: "<<path
            <<endl;
    }

    string line;
    while(std::getline(input_file, line))
    {
        if(!__cleanup_line(line))
            continue;

        if(__is_block_start(line))
        {
            std::vector<std::string> blocks;
            do {
                blocks.push_back(line);
            }while(std::getline(input_file, line) && !__is_block_end(line));

            __parse_block(blocks);
        }
        else {
            __parse_line(line);
        }
    }
}

void Cuts::Print()
{
    auto print_vector = [&](const std::vector<double> &v)
    {
        for(auto &i: v)
            std::cout<<std::setfill(' ')<<std::setw(16)<<i;
        std::cout<<std::endl;
    };

    auto print_block = [&](const block_t &b)
    {
        std::cout<<std::setfill(' ')<<std::setw(16)<<"module_name:"
                 <<std::setfill(' ')<<std::setw(16)<<b.module_name
                 <<std::endl;
        std::cout<<std::setfill(' ')<<std::setw(16)<<"layer_id:"
                 <<std::setfill(' ')<<std::setw(16)<<b.layer_id
                 <<std::endl;
        std::cout<<std::setfill(' ')<<std::setw(16)<<"position:";
        print_vector(b.position);
        std::cout<<std::setfill(' ')<<std::setw(16)<<"dimension:";
        print_vector(b.dimension);
        std::cout<<std::setfill(' ')<<std::setw(16)<<"offset:";
        print_vector(b.offset);
        std::cout<<std::setfill(' ')<<std::setw(16)<<"tilt_angle:";
        print_vector(b.tilt_angle);
    };

    cout<<"============================================================="<<endl;
    cout<<"cuts applied: "<<endl;
    cout<<"-------------------------------------------------------------"<<endl;
    for(auto &i: m_cache) {
        cout<<std::setfill(' ')<<std::setw(38)<<i.first;
        cout<<" : ";
        for(auto &j: i.second)
            cout<<std::setfill(' ')<<std::setw(10)<<j;
        cout<<endl;
    }
    cout<<"detector setup: "<<endl;
    cout<<"-------------------------------------------------------------"<<endl;
    for(auto &i: m_block) {
        cout<<i.first<<" = "<<endl;
        print_block(i.second);
    }
    cout<<"============================================================="<<endl;
}

////////////////////////////////////////////////////////////////////////////////
//                        helpers on hit information                          //
////////////////////////////////////////////////////////////////////////////////

int Cuts::__get_max_timebin(const StripHit &hit) const
{
    unsigned int TS = hit.ts_adc.size();
    if(TS <= 0) return -9999;

    int res = 0; float adc = hit.ts_adc[0];
    for(unsigned int i=1; i<TS; i++)
    {
        if(adc < hit.ts_adc[i]) {
            res = i;
            adc = hit.ts_adc[i];
        }
    }

    return res;
}

float Cuts::__get_sum_adc(const StripHit &hit) const
{
    unsigned int TS = hit.ts_adc.size();
    if(TS <= 0) return -9999;

    int res = 0;
    for(auto &i: hit.ts_adc)
    {
        res += i;
    }

    return res;
}

float Cuts::__get_avg_adc(const StripHit &hit) const
{
    unsigned int TS = hit.ts_adc.size();
    if(TS <= 0) return -99999.;

    float sum = __get_sum_adc(hit);
    float res = sum / (float) TS;

    return res;
}

float Cuts::__get_max_adc(const StripHit &hit) const
{
    unsigned int TS = hit.ts_adc.size();
    if(TS <= 0) return -99999.;

    int r = __get_max_timebin(hit);

    return hit.ts_adc[r];
}

/// adc averaged mean time
float Cuts::__get_mean_time(const StripHit &hit) const
{
    unsigned int TS = (hit.ts_adc).size();
    if(TS <= 0) return -99999.;

    float charge_sum = __get_sum_adc(hit);
    float mean_time = 0;

    for(unsigned int i=0; i<TS; i++)
    {
        mean_time += (hit.ts_adc)[i] * ((float)i+1.) * 25.0;
    }

    if(charge_sum != 0)
        mean_time /= charge_sum;
    else
        mean_time = 0.;

    return mean_time;
}

//
const ValueType &Cuts::__get(const std::string &str) const
{
    if(m_cut.find(str) == m_cut.end()) {
        std::cout<<"ERROR: Can't find cut :"<<str<<std::endl
                 <<"       please check config/gem_tracking.conf file."
                 <<"       make sure it is set up."<<std::endl;
    }

    return m_cut.at(str);
}

//
const ValueType &Cuts::__get(const char* str) const
{
    if(m_cut.find(str) == m_cut.end()) {
        std::cout<<"ERROR: Can't find cut :"<<str<<std::endl
                 <<"       please check config/gem_tracking.conf file."
                 <<"       make sure it is set up."<<std::endl;
    }

    return m_cut.at(str);
}

////////////////////////////////////////////////////////////////////////////////
//                    helpers on cluster information                          //
////////////////////////////////////////////////////////////////////////////////

int Cuts::__get_seed_strip_index(const StripCluster &c) const
{
    unsigned int cluster_size = c.hits.size();
    if(cluster_size <= 0) return -99999.;

    int max_strip = 0; float max_charge = c.hits[0].charge;
    for(unsigned int i=1; i<cluster_size; i++)
    {
        if(c.hits[i].charge > max_charge){
            max_charge = c.hits[i].charge;
            max_strip = i;
        }
    }

    return max_strip;
}

float Cuts::__get_seed_strip_max_adc(const StripCluster &c) const
{
    unsigned int cluster_size = c.hits.size();
    if(cluster_size <= 0) return -99999.;

    int max_strip = __get_seed_strip_index(c);

    float res = __get_max_adc(c.hits[max_strip]);
    return res;
}

float Cuts::__get_seed_strip_sum_adc(const StripCluster &c) const
{
    unsigned int cluster_size = c.hits.size();
    if(cluster_size <= 0) return -99999.;

    int max_strip = __get_seed_strip_index(c);

    float res = __get_sum_adc(c.hits[max_strip]);
    return res;
}

////////////////////////////////////////////////////////////////////////////////
//                                    cuts                                    //
////////////////////////////////////////////////////////////////////////////////

bool Cuts::max_time_bin(const StripHit &hit) const
{
    int timebin = __get_max_timebin(hit);

    for(auto &i: (m_cut.at("max time bin")).arr<int>())
    {
        if(i == timebin)
            return true;
    }

    return false;
}

bool Cuts::strip_mean_time(const StripHit &hit) const
{
    float mean_time = __get_mean_time(hit);

    auto mean_time_range = m_cut.at("strip mean time range").arr<float>();

    if(mean_time >= mean_time_range[0] && mean_time <= mean_time_range[1])
        return true;

    return false;
}

bool Cuts::reject_max_first_timebin(const StripHit &hit) const
{
    int max_bin = __get_max_timebin(hit);

    if(max_bin != 0)
        return true;

    if(m_cut.at("reject max first bin").val<bool>())
        return false;

    return true;
}

bool Cuts::reject_max_last_timebin(const StripHit &hit) const
{
    int max_bin = __get_max_timebin(hit);

    int n_ts = (int)hit.ts_adc.size() - 1;

    if(max_bin != n_ts)
        return true;

    if(m_cut.at("reject max last bin").val<bool>())
        return false;

    return true;
}

bool Cuts::seed_strip_min_peak_adc(const StripCluster &cluster) const
{
    float adc = __get_seed_strip_max_adc(cluster);

    float minimum_adc = m_cut.at("seed strip min peak ADC").val<float>();

    if(adc >= minimum_adc)
        return true;

    return false;
}

bool Cuts::seed_strip_min_sum_adc(const StripCluster &cluster) const
{
    float adc = __get_seed_strip_sum_adc(cluster);

    float minimum_adc = m_cut.at("seed strip min sum ADC").val<float>();

    if(adc >= minimum_adc)
        return true;
    return false;
}

bool Cuts::qualify_for_seed_strip(const StripHit &hit) const
{
    float peak_adc = __get_max_adc(hit);
    float sum_adc = __get_sum_adc(hit);

    float min_peak = m_cut.at("seed strip min peak ADC").val<float>();
    float min_sum = m_cut.at("seed strip min sum ADC").val<float>();

    if( (peak_adc >= min_peak) && (sum_adc >= min_sum))
        return true;

    return false;
}

bool Cuts::strip_mean_time_agreement(const StripHit &hit1, const StripHit &hit2) const
{
    float m1 = __get_mean_time(hit1);
    float m2 = __get_mean_time(hit2);

    float diff = abs(m1 - m2);

    float criteria = m_cut.at("strip mean time agreement").val<float>();

    if(diff <= criteria)
        return true;

    return false;
}

bool Cuts::time_sample_correlation_coefficient(const StripHit &hit1, const StripHit &hit2) const
{
    auto & bin_charge1 = hit1.ts_adc;
    auto & bin_charge2 = hit2.ts_adc;

    float correlation = __correlation_coefficient(bin_charge1, bin_charge2);
    float criteria = m_cut.at("time sample correlation coefficient").val<float>();

    if(correlation >= criteria)
        return true;
    return false;
}

bool Cuts::min_cluster_size(const StripCluster &cluster) const
{
    int cluster_size = (int)cluster.hits.size();

    int m = m_cut.at("min cluster size").val<int>();

    if(cluster_size >= m)
        return true;
    return false;
}

bool Cuts::cluster_adc_assymetry(const StripCluster &c1, const StripCluster &c2) const
{
    float c1_adc = c1.peak_charge;
    float c2_adc = c2.peak_charge;

    float assymetry = abs(c1_adc - c2_adc) / abs(c1_adc + c2_adc);
    
    float criteria = m_cut.at("2d cluster adc assymetry").val<float>();

    if(assymetry <= criteria)
        return true;

    return false;
}

bool Cuts::track_chi2([[maybe_unused]]const std::vector<StripCluster> &vc)
{
    return true;
}

bool Cuts::is_tracking_layer(const int &layer) const
{
	// if a layer not found, default it to participate tracking
	if(m_tracking_layer_switch.find(layer) == m_tracking_layer_switch.end())
	{
		std::cout<<"Cuts::Warning: layer "<<layer<<" tracking config not found. Default to true."
			<<std::endl;
		return true;
	}
	if(!m_tracking_layer_switch.at(layer))
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//                                 helpers                                    //
////////////////////////////////////////////////////////////////////////////////

// trim leading and trailing spaces
std::string Cuts::__trim_space(const string &s)
{
    if(s.size() <= 0)
        return string("");

    size_t pos1 = s.find_first_not_of(tokens);
    size_t pos2 = s.find_last_not_of(tokens);

    if(pos2 < pos1)
        return string("");

    size_t length = pos2 - pos1;

    return s.substr(pos1, length+1);
}

// remove comments
std::string Cuts::__remove_comments(const string &s)
{
    string res = s;

    size_t length = s.size();
    if(length <= 0)
        return res;

    // comments are leading by '#'
    size_t pos = s.find('#');

    if(pos == 0) {
        res.clear();
        return res;
    }

    if(pos == string::npos)
        return res;

    res = s.substr(0, pos);
    return res;
}

// clean up a line: remove leading and trailing spaces, remove comments
bool Cuts::__cleanup_line(std::string &s)
{
    std::string s1 = __remove_comments(s);
    s = __trim_space(s1);

    if(s.size() <= 0)
        return false;
    return true;
}

void Cuts::__parse_line(const std::string &line)
{
    string key;
    vector<string> val;

    __parse_key_value(line, key, val);

    if(key.size() > 0) {
        m_cache[key] = val;
    }
}

void Cuts::__parse_key_value(const string &line, string &key, vector<string> &val)
{
    // replace tokens with white space
    auto __replace_tokens = [&](const string & s) -> string
    {
        string l = s;
        char whitespace = ' ';
        for(char &c: tokens)
        {
            if(c == whitespace)
                continue;
            size_t pos = l.find(c);
            if(pos != string::npos) {
                l.replace(pos, 1, 1, whitespace);
            }
        }
        return l;
    };

    // separate key and values
    auto __separate_key_value_string = [&](const string &s, string &key, string &val)
    {
        size_t length = s.size();
        if(length <= 0)
            return;

        size_t pos = s.find('=');

        if(pos == string::npos || pos == 0 || pos == (length -1) )
        {
            cout<<"Warning:: format is incorrect, must be: \"key = value\" format."
                <<endl;
            return;
        }

        key = s.substr(0, pos);
        val = s.substr(pos+1, length - pos + 1);
    };

    // parse value list
    auto __separate_vals = [&](const string &s, vector<string> &res)
    {
        res.clear();
        string temp = __trim_space(s);
        temp = __replace_tokens(temp);

        istringstream iss(temp);
        string t;
        while(iss >> t)
        {
            res.push_back(t);
        }
    };

    // parse line
    string temp1 = __remove_comments(line);
    string temp2;
    __separate_key_value_string(temp1, key, temp2);
    key = __trim_space(key);
    __separate_vals(temp2, val);
}

void Cuts::__parse_block(const std::vector<std::string> &block)
{
    size_t block_length = block.size();

    std::string key; std::vector<std::string> value;
    std::unordered_map<std::string, ValueType> tmp;

    for(size_t i=1; i<block_length; i++)
    {
        __parse_key_value(block[i], key, value);
        if(key.size() > 0)
            tmp[key] = ValueType(value);
    }

    size_t sss = tmp.size();
    if(sss < 1) return;

    block_t block_data;
    block_data.layer_id = tmp.at("layer id").val<int>();
    block_data.position.clear();
    block_data.position = tmp.at("position").arr<double>();
    block_data.dimension.clear();
    block_data.dimension = tmp.at("dimension").arr<double>();
    block_data.offset.clear();
    block_data.offset = tmp.at("offset").arr<double>();
    block_data.tilt_angle.clear();
    block_data.tilt_angle = tmp.at("tilt angle").arr<double>();
    block_data.is_tracker = static_cast<bool>(tmp.at("participate tracking").val<int>());

    __parse_key_value(block[0], key, value);
    block_data.module_name = key;

    m_block[key] = block_data;

    // tracking layer config
    if(m_tracking_layer_switch.find(block_data.layer_id) != m_tracking_layer_switch.end())
    {
	    if(m_tracking_layer_switch.at(block_data.layer_id) != block_data.is_tracker)
	    {
		    std::cout<<"Cut::Error: conflicting tracking config found for layer: "
			    <<block_data.layer_id<<std::endl;
		    std::cout<<"            please check your config/gem_tracking.conf file."
			    <<std::endl;
		    exit(0);
	    }
    }
    else
    {
	    m_tracking_layer_switch[block_data.layer_id] = block_data.is_tracker;
    }
}

void Cuts::__convert_map()
{
    for(auto &i: m_cache)
    {
        ValueType val(i.second);
        m_cut[i.first] = val;
    }
}

bool Cuts::__is_block_start(const std::string & line)
{
    if(line.back() == '{')//(line.find("{") != std::string::npos)
        return true;
    return false;
}

bool Cuts::__is_block_end(const std::string & line)
{
    if(line[0] == '}')//(line.find("}") != std::string::npos)
        return true;
    return false;
}

float Cuts::__arr_mean(const vector<float> &v) const
{
    float res = 0;

    if(v.size() <= 0)
        return res;

    for(auto &i: v)
        res += i;

    int n = (int)v.size();

    return res / n;
}

float Cuts::__arr_sigma(const vector<float> &v) const
{
    float sigma = 0;

    if(v.size() <= 1)
        return sigma;

    float mean = __arr_mean(v);

    float square_sum = 0;
    for(auto &i: v)
        square_sum += pow(i - mean, 2);

    int n = (int)v.size();
    square_sum = square_sum / (n - 1);

    sigma = sqrt(square_sum);

    return sigma;
}

float Cuts::__correlation_coefficient(const vector<float> &v1,
        const vector<float> &v2) const
{
    float coefficient = 0;

    if(v1.size() != v2.size()) {
        cout<<"Warning:: correlation coefficient must be between two arrays with same length"
            <<endl; 
        return coefficient;
    }

    if(v1.size() <= 1) {
        cout<<"Warning:: correlation coefficient must be between two arrays with length > 1"
            <<endl; 
        return coefficient;
    }

    float cross_product = 0;
    float x_avg = __arr_mean(v1);
    float y_avg = __arr_mean(v2);

    for(size_t i=0; i<v1.size(); ++i)
    {
        cross_product += (v1[i] - x_avg) * (v2[i] - y_avg);
    }

    float x_sigma = __arr_sigma(v1);
    float y_sigma = __arr_sigma(v2);

    cross_product = cross_product / (x_sigma * y_sigma);

    int n = (int)v1.size();
    coefficient = cross_product / (n - 1);
    return coefficient;
}

bool Cuts::cluster_strip_time_agreement(const StripCluster &c) const
{
    int seed = __get_seed_strip_index(c);

    unsigned int cluster_size = c.hits.size();
    for(unsigned int i=0; i<cluster_size && i!=(unsigned int)seed; i++)
    {
        if(!strip_mean_time_agreement(c.hits[seed], c.hits[i]))
            return false;
    }

    return true;
}

bool Cuts::cluster_time_assymetry(const StripCluster &c1, const StripCluster &c2) const
{
    int seed1 = __get_seed_strip_index(c1);
    int seed2 = __get_seed_strip_index(c2);

    if(seed1 < 0 || seed2 < 0)
        return false;

    if(!strip_mean_time_agreement(c1.hits[seed1], c2.hits[seed2]))
        return false;

    return true;
}

void Cuts::__print_strip(const StripHit &hit) const
{
    std::cout<<"strip: "
        <<setfill(' ')<<setw(6)<<hit.strip
        <<setfill(' ')<<setw(6)<<hit.charge
        <<setfill(' ')<<setw(6)<<hit.position
        <<setfill(' ')<<setw(6)<<hit.cross_talk
        <<std::endl;
    std::cout<<"apv address: "<<hit.apv_addr<<std::endl;
    for(auto &i: hit.ts_adc)
        std::cout<<setfill(' ')<<setw(6)<<i;
    std::cout<<std::endl;
}

void Cuts::__print_cluster(const StripCluster &c) const
{
    std::cout<<"cluster: "
        <<setfill(' ')<<setw(6)<<c.position
        <<setfill(' ')<<setw(6)<<c.peak_charge
        <<setfill(' ')<<setw(6)<<c.total_charge
        <<setfill(' ')<<setw(6)<<c.cross_talk
        <<std::endl;
    std::cout<<"strips: "<<std::endl;
    for(auto &i: c.hits)
        __print_strip(i);
}
