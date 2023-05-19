#include <iostream>
#include "TrackingUtility.h"

namespace tracking_dev {

TrackingUtility::TrackingUtility()
{
}

TrackingUtility::~TrackingUtility()
{
}

// fitting - calculate fitted line:
//     points must be global coordinate
//     xtrack, ytrack are projected coords at z = 0
//     xptrack, yptrack are the slope in x-z and y-z plane
void TrackingUtility::line_of_best_fit(const std::vector<point_t> &points, double &xtrack, double &ytrack,
            double &xptrack, double &yptrack)
{
    double sumx = 0., sumy = 0., sumz = 0., sumxz = 0., sumyz = 0., sumz2 = 0.;

    for(auto &i: points) {
        sumx += i.x;
        sumy += i.y;
        sumz += i.z;
        sumxz += i.x * i.z;
        sumyz += i.y * i.z;
        sumz2 += i.z * i.z;
    }

    double nhits = (double)points.size();
    double denominator = (sumz2 * nhits - sumz * sumz);

    xptrack = (nhits * sumxz - sumx * sumz) / denominator;
    yptrack = (nhits * sumyz - sumy * sumz) / denominator;
    xtrack = (sumx * sumz2 - sumxz * sumz) / denominator;
    ytrack = (sumy * sumz2 - sumyz * sumz) / denominator;
}

// calculate track projected point at z plane
// track starting point @pt_track, direction @dir_track
point_t TrackingUtility::projected_point(const point_t &pt_track, const point_t &dir_track,
        const double &z)
{
    point_t zaxis(0, 0, 1);
    double r = (z - pt_track.z)/zaxis.dot(dir_track);

    return pt_track + dir_track * r;
}

// calculate track intersection point at z plane
// the connected two points are p1 and p2
point_t TrackingUtility::intersection_point(const point_t &p1, const point_t &p2,
        const double &z)
{
    point_t dir = p2 - p1;
    dir = dir.unit();

    return projected_point(p1, dir, z);
}

// get all tracking parameters
void TrackingUtility::FitLine(const std::vector<point_t> &points, double &xtrack, double &ytrack,
        double &xptrack, double &yptrack, double &chi2ndf, std::vector<double> &xresid,
        std::vector<double> &yresid, double xreso, double yreso)
{
    line_of_best_fit(points, xtrack, ytrack, xptrack, yptrack);

    double chi2 = 0.;
    xresid.clear();
    yresid.clear();

    double x_resolution = xreso;
    double y_resolution = yreso;

    point_t pt_track(xtrack, ytrack, 0);
    point_t _slope_track(xptrack, yptrack, 1.);
    point_t slope_track = _slope_track.unit();

    for(auto &i: points)
    {
        point_t temp = projected_point(pt_track, slope_track, i.z);

        xresid.push_back(temp.x - i.x);
        yresid.push_back(temp.y - i.y);

        chi2 += (temp.x - i.x) * (temp.x - i.x) / x_resolution / x_resolution +
            (temp.y - i.y) * (temp.y - i.y) / y_resolution / y_resolution;
    }

    double ndf = 2. * (double)points.size() - 4;
    if(ndf <= 0) ndf = 1.;

    chi2ndf = chi2 / ndf;
}

// unit test
void TrackingUtility::UnitTest()
{
    std::cout<<"Unit Test for tracking base class."<<std::endl;
}

};
