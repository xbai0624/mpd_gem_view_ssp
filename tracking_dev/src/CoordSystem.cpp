#include "CoordSystem.h"
#include "Cuts.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace tracking_dev {
    CoordSystem::CoordSystem()
    {
        Init();
    }

    CoordSystem::~CoordSystem()
    {
    }

    void CoordSystem::Reload()
    {
        // clear cached per-detector geometry, then re-read from Cuts
        // (the caller is expected to Cuts::Reload() first)
        offset_gem.clear();
        angle_gem.clear();
        position_gem.clear();
        dimension_gem.clear();
        tracker_config_gem.clear();

        Init();
    }

    void CoordSystem::Init()
    {
        // Bounded read of a 3-component vector. Throws a useful message
        // (block name + field name + missing index) instead of letting
        // std::vector::at() throw "vector::_M_range_check" -- the GUI
        // Apply slot catches this and shows a dialog.
        auto need = [](const std::vector<double> &a, std::size_t i,
                       const char *what, const std::string &block) -> double
        {
            if(i < a.size()) return a[i];
            throw std::runtime_error(std::string("CoordSystem: '") + block
                    + "' is missing component " + std::to_string(i)
                    + " of '" + what + "' in gem_tracking.conf");
        };

        auto gem_setup = Cuts::Instance().__get_block_data();
        for(auto &i: gem_setup)
        {
            const std::string &name = i.first;          // e.g. "gem0"
            const auto &b = i.second;
            int module_id = b.module_id;

            offset_gem[module_id]    = point_t(need(b.offset,     0, "offset",     name),
                                               need(b.offset,     1, "offset",     name),
                                               need(b.offset,     2, "offset",     name));
            angle_gem[module_id]     = point_t(need(b.tilt_angle, 0, "tilt angle", name),
                                               need(b.tilt_angle, 1, "tilt angle", name),
                                               need(b.tilt_angle, 2, "tilt angle", name));
            dimension_gem[module_id] = point_t(need(b.dimension,  0, "dimension",  name),
                                               need(b.dimension,  1, "dimension",  name),
                                               need(b.dimension,  2, "dimension",  name));
            position_gem[module_id]  = point_t(need(b.position,   0, "position",   name),
                                               need(b.position,   1, "position",   name),
                                               need(b.position,   2, "position",   name));

            tracker_config_gem[module_id] = b.is_tracker;
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
                   (c_beta*s_alpha) * p.y +
                   (c_beta*c_alpha) * local_z;

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

    void CoordSystem::Transform(point_t &p, int det_id)
    {
        Transform(p, angle_gem[det_id], offset_gem[det_id]);
    }
};
