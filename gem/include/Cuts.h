#ifndef CUTS_H
#define CUTS_H

#include <string>
#include <unordered_map>
#include <vector>

#include "ValueType.h"

class StripHit;
class StripCluster;

class Cuts
{
    public:
        Cuts(){Init("config/gem_tracking.conf");}
        ~Cuts();

        // members
        void SetFile(const char* path);
        void LoadFile();
        void Init(const char* path);
        void Print();

    private:
        void __parse_key_value(const std::string &line, 
                std::string &key, std::vector<std::string> &val);
        void __parse_line(const std::string &);
        void __parse_block(const std::vector<std::string> &block);
        void __convert_map();
        bool __is_block_start(const std::string &);
        bool __is_block_end(const std::string &);
        std::string __trim_space(const std::string &s);
        std::string __remove_comments(const std::string &s);

        // helpers
        float __arr_mean(const std::vector<float> &v) const;
        float __arr_sigma(const std::vector<float> &v) const;
        float __correlation_coefficient(const std::vector<float> &v1,
                const std::vector<float> &v2) const;
        void __print_strip(const StripHit &hit) const;
        void __print_cluster(const StripCluster &c) const;

        // getters
        int __get_max_timebin(const StripHit &hit) const;
        float __get_sum_adc(const StripHit &hit) const;
        float __get_avg_adc(const StripHit &hit) const;
        float __get_max_adc(const StripHit &hit) const;
        float __get_mean_time(const StripHit &hit) const;
        int __get_seed_strip_index(const StripCluster &c) const;
        float __get_seed_strip_max_adc(const StripCluster &c) const;
        float __get_seed_strip_sum_adc(const StripCluster &c) const;

    public:
        // getters
        const ValueType &__get(const std::string &str) const;
        const ValueType &__get(const char* str) const;

        // cuts on hits
        bool max_time_bin(const StripHit &) const;
        bool strip_mean_time(const StripHit &) const;
        bool reject_max_first_timebin(const StripHit &) const;
        bool reject_max_last_timebin(const StripHit &) const;

        // cuts on clusters
        bool seed_strip_min_peak_adc(const StripCluster &) const;
        bool seed_strip_min_sum_adc(const StripCluster &) const;
        bool qualify_for_seed_strip(const StripHit &) const;
        // --between seed strip and any single constituent strip
        bool strip_mean_time_agreement(const StripHit &, const StripHit &) const;
        bool time_sample_correlation_coefficient(const StripHit &, const StripHit &) const;
        // --on total number of strips
        bool min_cluster_size(const StripCluster &) const;
        // --timing correlation between any strip and seed strip
        bool cluster_strip_time_agreement(const StripCluster &c) const;

        // cuts on cluster matching
        bool cluster_adc_assymetry(const StripCluster &c1, const StripCluster &c2) const;
        // cuts on two cluster timing agreement
        bool cluster_time_assymetry(const StripCluster &c1, const StripCluster &c2) const;
 
        // cuts on tracking
        bool track_chi2(const std::vector<StripCluster> &);

    public:
        struct block_t {
            int layer_id;
            std::vector<double> position;
            std::vector<double> dimension;
            std::vector<double> offset;
            std::vector<double> tilt_angle;

            block_t() : layer_id(0)
            {
                position.clear(); dimension.clear();
                offset.clear(); tilt_angle.clear();
            }
        };
        const std::unordered_map<std::string, block_t> & __get_block_data() const {return m_block;}

    private:
        std::string path;

        std::string tokens = " ,;:@()\'\"\r";

        // normal entries
        std::unordered_map<std::string, std::vector<std::string>> m_cache;
        std::unordered_map<std::string, ValueType> m_cut;

        // block entries : within '{' and '}'
        std::unordered_map<std::string, block_t> m_block;
};

#endif
