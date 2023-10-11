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
        // refer to: https://en.wikipedia.org/wiki/Rotation_matrix
        // R = RzRyRx
        double c_gamma = std::cos(rot.x), s_gamma = std::sin(rot.x);
        double c_beta = std::cos(rot.y), s_beta = std::sin(rot.y);
        double c_alpha = std::cos(rot.z), s_alpha = std::sin(rot.z);

        double local_z = 0.; // rotation is in gem local coordinates
        double x = (c_beta*c_gamma) * p.x +
                   (s_alpha*s_beta*c_gamma - c_alpha*s_gamma) * p.y +
                   (c_alpha*s_beta*c_gamma + s_alpha*s_gamma) * local_z;

        double y = (c_beta*s_gamma) * p.x +
                   (s_alpha*s_beta*s_gamma + c_alpha*c_gamma) * p.y +
                   (c_alpha*s_beta*s_gamma - s_alpha*c_gamma) * local_z;

        double z = (-s_beta) * p.x +
                   (c_beta*s_gamma) * p.y +
                   (c_beta*c_gamma) * local_z;

        p.x = x; p.y = y; p.z = z + p.z;
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
