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

            // second order offset
            point_t SO_offset(
                i.second.second_order_offset.at(0), 
                i.second.second_order_offset.at(1), 
                i.second.second_order_offset.at(2)
                ); 
            SO_offset_gem[layer] = SO_offset; 

            // second order global angle
            point_t SO_tilt_angle(
                i.second.second_order_tilt_angle.at(0),
                i.second.second_order_tilt_angle.at(1), 
                i.second.second_order_tilt_angle.at(2)
                );
            SO_angle_gem[layer] = SO_tilt_angle; 

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

        // Minh: Assuming extrinsic rotation within the detector local frame

        double c_gamma = std::cos(rot.z); 
	    double s_gamma = std::sin(rot.z);

        double c_beta = std::cos(rot.y); 
	    double s_beta = std::sin(rot.y);

        double c_alpha = std::cos(rot.x);
	    double s_alpha = std::sin(rot.x);
	
        double local_x = p.x; // Assuming xy coordinates are same regardless local/global frame
        double local_y = p.y; 
        double local_z = 0; // Set detector coordinates in local frame 
        
	    double x =  (c_beta * c_gamma) * local_x +
                    (s_alpha * s_beta * c_gamma - c_alpha * s_gamma) * local_y +
                    (c_alpha * s_beta * c_gamma + s_alpha * s_gamma) * local_z;

    	double y =  (c_beta * s_gamma) * local_x +
                    (s_alpha * s_beta * s_gamma + c_alpha * c_gamma) * local_y +
                    (c_alpha * s_beta * s_gamma - s_alpha * c_gamma) * local_z;

    	double z =  (-s_beta) * local_x +
                    (c_beta * s_alpha) * local_y + 
                    (c_alpha * c_beta) * local_z;

        // Reset hits to global frame 
        p.x = x; 
        p.y = y; 
        p.z = p.z + z; 
    }

void CoordSystem::Global_Rotate(point_t & p, const point_t &rot)
    {
        // refer to: https://en.wikipedia.org/wiki/Rotation_matrix
        // R = RzRyRx
        // Global rotation function is primarily for second order rotation parameters

        double c_gamma = std::cos(rot.z); 
	    double s_gamma = std::sin(rot.z);

        double c_beta = std::cos(rot.y); 
	    double s_beta = std::sin(rot.y);

        double c_alpha = std::cos(rot.x);
	    double s_alpha = std::sin(rot.x);
	
        double local_x = p.x; 
        double local_y = p.y; 
        double local_z = p.z; // Rotation is now global coordinates
        
	    double x =  (c_beta * c_gamma) * local_x +
                    (s_alpha * s_beta * c_gamma - c_alpha * s_gamma) * local_y +
                    (c_alpha * s_beta * c_gamma + s_alpha * s_gamma) * local_z;

    	double y =  (c_beta * s_gamma) * local_x +
                    (s_alpha * s_beta * s_gamma + c_alpha * c_gamma) * local_y +
                    (c_alpha * s_beta * s_gamma - s_alpha * c_gamma) * local_z;

    	double z =  (-s_beta) * local_x +
                    (c_beta * s_alpha) * local_y + 
                    (c_alpha * c_beta) * local_z;

        p.x = x; p.y = y; p.z = z;
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

    void CoordSystem::Transform(point_t &p, const point_t &rot, const point_t &t, const point_t &SO_rot, const point_t &SO_t)
    {
        // First, do systematic FIRST ORDER angle correction (Extrinsic rotation)
        Rotate(p, rot);

        // Second, do FIRST ORDER offset correction (Done in the global frame) 
        Translate(p, t);

        // Third, do SECOND ORDER global angle correction (Extrinsic rotation)
        Global_Rotate(p, SO_rot); 

        // Fourth, do SECOND ORDER offset correction (Done in the global frame)
        Translate(p, SO_t);
    }

    void CoordSystem::Transform(point_t &p, int ilayer)
    {
        Transform(p, angle_gem[ilayer], offset_gem[ilayer], SO_angle_gem[ilayer], SO_offset_gem[ilayer]);
    }
};
