#ifndef TOY_MODEL_H
#define TOY_MODEL_H

/* 
 * generate toy model data for testing alignment algorithm
 * 
 */

#include "tracking_struct.h"
#include <vector>

namespace tracking_dev {
    class ToyModel
    {
    public:
        struct event_t
        {
            std::vector<point_t> hits;
        };

    public:
        ToyModel();
        ~ToyModel();

        void Generate();
        void Load();
        void WriteTextFile(const std::vector<event_t> & events, const char* path);
        const std::vector<event_t> & GetAllEvents(){return all_events;}

    private:
        std::vector<event_t> all_events;
        std::vector<event_t> all_true_events; // for debug
    };
};

#endif
