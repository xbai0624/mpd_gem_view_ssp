#include "CoordSystem.h"
#include <cmath>
#include <iostream>
#include <iomanip>

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

            // is in tracker system or not
            tracker_config_gem[layer] = i.second.is_tracker;
        }
    }

    void CoordSystem::Rotate(point_t & p, const point_t &rot)
    {
        // refer to: https://en.wikipedia.org/wiki/Rotation_matrix
        // R = RzRyRx
	
	/* Diagnostic
	std::cout << std::setprecision(15);
        std::cout << "Initial Coordinates: " << std::endl;
        std::cout << "x: " << p.x << " y: " << p.y << " z: " << p.z << std::endl;
	*/ 

        double c_gamma = std::cos(rot.z); 
	double s_gamma = std::sin(rot.z);

        double c_beta = std::cos(rot.y); 
	double s_beta = std::sin(rot.y);

        double c_alpha = std::cos(rot.x);
	double s_alpha = std::sin(rot.x);
	
	double local_x = p.x; 
	double local_y = p.y; 
	double local_z = 0.; //Original code implemented by xinzhan (rotation is in gem local coord)
        
	double x = (c_beta * c_gamma) * local_x +
               (s_alpha * s_beta * c_gamma - c_alpha * s_gamma) * local_y;

    	double y = (c_beta * s_gamma) * local_x +
               (s_alpha * s_beta * s_gamma + c_alpha * c_gamma) * local_y;

    	double z = (-s_beta) * local_x +
               (c_beta * s_alpha) * local_y;

	/* Diagnostic
	// Debug print statements
        std::cout << std::setprecision(15);
        std::cout << "Intermediate rotation results: " << std::endl;
        std::cout << "x: " << x << " y: " << y << " z: " << z << std::endl;
	*/

        p.x = x; p.y = y; p.z = p.z + z;

	// Diagnostics
	// Debug print the final rotated point
        //std::cout << "Rotated point: (" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
    
    }


    void CoordSystem::Translate(point_t &p, const point_t &t)
    {
        p.x = p.x + t.x;
        p.y = p.y + t.y;
        p.z = p.z + t.z;

	// Diagnostics
	// Debug print the final translated point
        //std::cout << "Translated point: (" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
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
