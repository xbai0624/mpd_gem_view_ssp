#include "CoordSystem.h"
#include <cmath>

namespace tracking_dev {
    CoordSystem::CoordSystem()
    {
        Init();
    }

    CoordSystem::~CoordSystem()
    {
    }

    void CoordSystem::Init()
    {
        gem_cuts = new Cuts();

        auto gem_setup = gem_cuts -> __get_block_data();
        for(auto &i: gem_setup)
        {
            int layer = i.second.layer_id;

            // offset
            point_t offset(
                    i.second.offset.at(0),
                    i.second.offset.at(1),
                    i.second.offset.at(2)
                    );
            offset_gem[layer] = offset;

            // tilt angle
            point_t angle(
                    i.second.tilt_angle.at(0),
                    i.second.tilt_angle.at(1),
                    i.second.tilt_angle.at(2)
                    );
            angle_gem[layer] = angle;

            // size
            point_t dim(
                    i.second.dimension.at(0),
                    i.second.dimension.at(1),
                    i.second.dimension.at(2)
                    );
            dimension_gem[layer] = dim;

            // position
            point_t pos(
                    i.second.position.at(0),
                    i.second.position.at(1),
                    i.second.position.at(2)
                    );
            position_gem[layer] = pos;
        }
    }

    void CoordSystem::Rotate(point_t & p, const point_t &rot)
    {
        // Rxyz = RxRyRz
        double cx = std::cos(rot.x), sx = std::sin(rot.x);
        double cy = std::cos(rot.y), sy = std::sin(rot.y);
        double cz = std::cos(rot.z), sz = std::sin(rot.z);

        double x = cy*cz*p.x - cy*sz*p.y + sy*p.z;
        double y = (cx*sz + sx*sy*cz)*p.x + (cx*cz - sx*sy*sz)*p.y - sx*cy*p.z;
        double z = (sx*sz - cx*sy*cz)*p.x + (sx*cz + cx*sy*sz)*p.y + cx*cy*p.z;

        p.x = x; p.y = y; p.z = z;
    }

    void CoordSystem::Translate(point_t &p, const point_t &t)
    {
        p.x = p.x + t.x;
        p.y = p.y + t.y;
        p.z = p.z + t.z;
    }

    void CoordSystem::Transform(point_t &p, const point_t &rot, const point_t &t)
    {
        // first do angle correction
        Rotate(p, rot);

        // second to offset correction
        Translate(p, t);
    }

    void CoordSystem::Transform(point_t &p, int ilayer)
    {
        Transform(p, angle_gem[ilayer], offset_gem[ilayer]);
    }
};
