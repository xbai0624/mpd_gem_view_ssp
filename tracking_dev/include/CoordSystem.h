#ifndef COORD_SYSTEM_H
#define COORD_SYSTEM_H

#include "tracking_struct.h"
#include "Cuts.h"

namespace tracking_dev
{
    class CoordSystem 
    {
    public:
        CoordSystem();
        ~CoordSystem();

        void Init();
        void PassCutsHandle(Cuts *c){gem_cuts = c;}

        void Rotate(point_t &p, const point_t &rot);
        void Translate(point_t &p, const point_t &t);
        void Transform(point_t &p, const point_t &rot, const point_t &t);
        void Transform(point_t &p, int ilayer);

        // getters
        point_t GetLayerOffset(int i){return offset_gem.at(i);}
        point_t GetLayerTiltAngle(int i){return angle_gem.at(i);}
        point_t GetLayerPosition(int i){return position_gem.at(i);}
        point_t GetLayerDimension(int i){return dimension_gem.at(i);}
        Cuts* GetCutsHandle(){return gem_cuts;}

    private:
        Cuts *gem_cuts;

        std::unordered_map<int, point_t> offset_gem;
        std::unordered_map<int, point_t> angle_gem;
        std::unordered_map<int, point_t> position_gem;
        std::unordered_map<int, point_t> dimension_gem;
    };
};

#endif
