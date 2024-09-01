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
        void Global_Rotate(point_t &p, const point_t &rot); // Global rotation function for second order alignment
        void Translate(point_t &p, const point_t &t);
        void Transform(point_t &p, const point_t &rot, const point_t &t, const point_t &SO_rot, const point_t &SO_t);
        void Transform(point_t &p, int ilayer);

        // getters
        point_t GetLayerOffset(int i){return offset_gem.at(i);}
        point_t GetLayerTiltAngle(int i){return angle_gem.at(i);}
        point_t GetLayerPosition(int i){return position_gem.at(i);}
        point_t GetLayerDimension(int i){return dimension_gem.at(i);}
        point_t GetSecondOrderLayerOffset(int i){return SO_offset_gem.at(i);}
        point_t GetSecondOrderLayerTiltAngle(int i){return SO_angle_gem.at(i);}
        bool IsInTrackerSystem(int i){return tracker_config_gem.at(i);}
        Cuts* GetCutsHandle(){return gem_cuts;}

    private:
        Cuts *gem_cuts;

        std::unordered_map<int, point_t> offset_gem;
        std::unordered_map<int, point_t> SO_offset_gem; // For second order translation
        std::unordered_map<int, point_t> angle_gem;
        std::unordered_map<int, point_t> SO_angle_gem; // For second order global rotation
        std::unordered_map<int, point_t> position_gem;
        std::unordered_map<int, point_t> dimension_gem;
        std::unordered_map<int, bool> tracker_config_gem;
    };
};

#endif
