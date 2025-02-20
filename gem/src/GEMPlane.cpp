//============================================================================//
// GEM plane class                                                            //
// A GEM plane is a component of GEM detector, it should never exist without  //
// a detector, thus the memory of plane will be managed in GEM detector       //
//                                                                            //
// A GEM plane can be connected to several APV units                          //
// GEM hits are collected and grouped on plane level                          //
//                                                                            //
// Chao Peng, Frame work of this class                                        //
// Xinzhan Bai, position and charge calculation method                        //
// 10/07/2016                                                                 //
// Xinzhan Bai, adapted to mpd system                                         //
// 12/02/2020
//============================================================================//

#include <functional>

#include "GEMPlane.h"
#include "GEMDetector.h"
#include "GEMDetectorLayer.h"
#include "GEMCluster.h"

////////////////////////////////////////////////////////////////////////////////
// constructor

GEMPlane::GEMPlane(GEMDetector *det)
: detector(det), name("Undefined"), type(Plane_X), size(0.), orient(0)
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// constructor

GEMPlane::GEMPlane(const std::string &n, const int &t, const float &s,
                           const int &c, const int &o, const int &d, GEMDetector *det)
: detector(det), name(n), size(s), orient(o), direction(d)
{
    type = (Type)t;

    apv_list.resize(c, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// copy constructor
// connections between it and apv/detector won't be copied

GEMPlane::GEMPlane(const GEMPlane &that)
: detector(nullptr), name(that.name), type(that.type), size(that.size), orient(that.orient),
  direction(that.direction), strip_hits(that.strip_hits), strip_clusters(that.strip_clusters)
{
    apv_list.resize(that.apv_list.size(), nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// move constructor

GEMPlane::GEMPlane(GEMPlane &&that)
: detector(nullptr), name(std::move(that.name)), type(that.type), size(that.size),
  orient(that.orient), direction(that.direction), strip_hits(std::move(that.strip_hits)),
  strip_clusters(std::move(that.strip_clusters))
{
    apv_list.resize(that.apv_list.size(), nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// destructor

GEMPlane::~GEMPlane()
{
    UnsetDetector();
    DisconnectAPVs();
}

////////////////////////////////////////////////////////////////////////////////
// copy assignment

GEMPlane &GEMPlane::operator =(const GEMPlane &rhs)
{
    if(this == &rhs)
        return *this;

    GEMPlane that(rhs);
    *this = std::move(that);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// move assignment

GEMPlane &GEMPlane::operator =(GEMPlane &&rhs)
{
    if(this == &rhs)
        return *this;

    name = std::move(rhs.name);
    type = rhs.type;
    size = rhs.size;
    orient = rhs.orient;
    direction = rhs.direction;

    strip_hits = std::move(rhs.strip_hits);
    strip_clusters = std::move(rhs.strip_clusters);
    return *this;
}


////////////////////////////////////////////////////////////////////////////////
// set detector to the plane

void GEMPlane::SetDetector(GEMDetector *det, bool force_set)
{
    if(det == detector)
        return;

    if(!force_set)
        UnsetDetector();

    detector = det;
}

////////////////////////////////////////////////////////////////////////////////
// disconnect the detector

void GEMPlane::UnsetDetector(bool force_unset)
{
    if(!detector)
        return;

    if(!force_unset)
        detector->DisconnectPlane(type, true);

    detector = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// change the capacity

void GEMPlane::SetCapacity(int c)
{
    // capacity cannot be negative
    if(c < 0) c = 0;

    if((uint32_t)c < apv_list.size())
    {
        std::cout << " GEM Plane Warning: Reduce the connectors on plane "
                  << name << " from " << apv_list.size() << " to " << c
                  << ". Thus it will lose the connection between APVs that beyond "
                  << c
                  << std::endl;

        for(uint32_t i = c; i < apv_list.size(); ++i)
        {
            if(apv_list[i] != nullptr)
                apv_list[i]->UnsetDetectorPlane(true);
        }
    }

    apv_list.resize(c, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// connect an APV to the plane

void GEMPlane::ConnectAPV(GEMAPV *apv, const int &index)
{
    if(apv == nullptr)
        return;

    if((uint32_t)index >= apv_list.size()) {
        std::cout << " GEM Plane Warning: Failed to connect plane " << name
                  << " with APV " << apv->GetAddress()
                  << ". Plane connectors are not enough, have " << apv_list.size()
                  << ", this APV is to be connected at " << index
                  << std::endl;
        return;
    }

    if(apv_list[index] != nullptr) {
        std::cout << " GEM Plane Warning: The connector " << index
                  << " of plane " << name << " is connected to APV: " << apv->GetAddress()
                  << ", replace the connection: " << apv_list[index] -> GetAddress()
                  << std::endl;
        return;
    }

    apv_list[index] = apv;
    apv->SetDetectorPlane(this, index);
}


////////////////////////////////////////////////////////////////////////////////
// disconnect an APV

void GEMPlane::DisconnectAPV(const uint32_t &plane_index, bool force_disconn)
{
    if(plane_index >= apv_list.size())
        return;

    auto &apv = apv_list[plane_index];

    if(!apv)
        return;

    if(!force_disconn)
        apv->UnsetDetectorPlane(true);

    apv = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// reset all APV connections

void GEMPlane::DisconnectAPVs()
{
    for(auto &apv : apv_list)
    {
        if(apv) {
            apv->UnsetDetectorPlane(true);
            apv = nullptr;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// get existing APV list

std::vector<GEMAPV*> GEMPlane::GetAPVList()
const
{
    // since the apv list may contain nullptr,
    // only pack connected APVs and return
    std::vector<GEMAPV*> result;

    for(const auto &apv : apv_list)
    {
        if(apv != nullptr)
            result.push_back(apv);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////
// calculate strip position by plane strip index
//
// x direction is the horizontal one, with 4 chambers overlapping each other
// y direction is the vertical one, with 4 chambers arranged one by one

float GEMPlane::GetStripPosition(const int &plane_strip)
    const
{
    float position;

    // layer_index is reserved, in case there's overlapping between two 
    // adjacent chamber, in that case layer_index will be used
    [[maybe_unused]]int layer_index = detector -> GetDetLayerPositionIndex();

    float layer_det_capacity = detector -> GetLayer() -> GetNumberOfDetectorsInLayer();
    float STRIP_PITCH = detector -> GetLayer() -> GetChamberPlanePitch(type);

    const std::string &detector_type = detector -> GetType();

    // xy gem chambers
    auto xy = [&]() {
        // suppose there's no overlapping area between each chambers
        // otherwise this needs to be modified

        // size: the total length of one gem chamber plane, 
        //       determined from number of apvs and strip pitch,
        //       size * layer_det_capacity = the length of the entire gem detector layer

        // plane_strip: is layer oriented (all gem detectors in one layer are combined)

        // position: is in XY orthogonal coordinate system
        if(type == Plane_X)
        {
            position = -0.5*(size * layer_det_capacity - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5*(size - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& y_offset = detector -> GetLayer() -> GetYOffset();
            position += y_offset;
        }
    };

    // uv gem chambers
    auto sbs_uv = [&]() {
        // for sbs uv gem chamber, u side and v side both have 30 apvs,
        // the layout of u strips and v strips are the same

        // position: is in UV coordinate system, not XY orthogonal coordinate system
        //           need to convert to XY orthogonal system during matching, 
        //           DO NOT convert in here
        if(type == Plane_X) 
        {
            position = -0.5 * (size - STRIP_PITCH) + STRIP_PITCH * plane_strip;
            const double &x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5 * (size - STRIP_PITCH) + STRIP_PITCH * plane_strip;
            const double &y_offset = detector -> GetLayer() -> GetXOffset();
            position += y_offset;
        }
    };

    // xw gem chambers
    auto sbs_xw = [&]() {
	// for sbs xw gem chamber, x is the shorter strips, they are parallel to the short
	// edge of the detector frame
	// w strips has 45 degree angle

	// positon: is in XW coordinates, not XY orthogonal coordinates
	//          need to convert to XY during matching
	//          DO NOT convert in here
	if(type == Plane_X)
        {
            position = -0.5*(size * layer_det_capacity - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5*(size - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& y_offset = detector -> GetLayer() -> GetYOffset();
            position += y_offset;
        }
    };

    // moller gem chambers
    auto moller_uv = [&]() {
        // for moller uv gem chamber, u side and v side both have 5 apvs,
        // the layout of u strips and v strips are the same

        // position: is in UV coordinate system, not XY orthogonal coordinate system
        //           need to convert to XY orthogonal system during matching, 
        //           DO NOT convert in here
        if(type == Plane_X) 
        {
            position = -0.5 * (size - STRIP_PITCH) + STRIP_PITCH * plane_strip;
            const double &x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5 * (size - STRIP_PITCH) + STRIP_PITCH * plane_strip;
            const double &y_offset = detector -> GetLayer() -> GetXOffset();
            position += y_offset;
        }
    };

    // Hall D gem chambers
    auto halld_xy = [&]() {
        // suppose there's no overlapping area between each chambers
        // otherwise this needs to be modified

        // size: the total length of one gem chamber plane, 
        //       determined from number of apvs and strip pitch,
        //       size * layer_det_capacity = the length of the entire gem detector layer

        // plane_strip: is layer oriented (all gem detectors in one layer are combined)

        // position: is in XY orthogonal coordinate system
        if(type == Plane_X)
        {
            position = -0.5*(size * layer_det_capacity - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5*(size - STRIP_PITCH) + STRIP_PITCH*((int)(plane_strip/2));
            const double& y_offset = detector -> GetLayer() -> GetYOffset();
            position += y_offset;
        }
    };

    // PRad gem chambers
    // For PRad GEM: Y Axis (1) must be the long side, with two 12-slot backplanes
    //               X Axis (0) must be the short side, with one 5-slot, one 7-slot backplanes
    auto prad_xy = [&]() {
        // suppose there's no overlapping area between each chambers
        // otherwise this needs to be modified

        // size: the total length of one gem chamber plane, 
        //       determined from number of apvs and strip pitch,
        //       size * layer_det_capacity = the length of the entire gem detector layer

        // plane_strip: is layer oriented (all gem detectors in one layer are combined)

        // position: is in XY orthogonal coordinate system
        if(type == Plane_X)
        {
            position = -0.5*(size * layer_det_capacity + 31*STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& x_offset = detector -> GetLayer() -> GetXOffset();
            position += x_offset;
        } else {
            position = -0.5*(size - STRIP_PITCH) + STRIP_PITCH*plane_strip;
            const double& y_offset = detector -> GetLayer() -> GetYOffset();
            position += y_offset;
        }
    };

    // re-organize using maps, this is to cut off the time spent on string comparison
    const std::unordered_map<std::string, std::function<void()>> get_position = {
        {"UVAXYGEM", xy},
        {"UVAXWGEM", sbs_xw},
        {"INFNXYGEM", xy},
        {"INFNXWGEM", sbs_xw},
        {"UVAUVGEM", sbs_uv},
        {"MOLLERGEM", moller_uv},
        {"HALLDGEM", halld_xy},
        {"PRADGEM", prad_xy}
    };

    // calculate strip position
    if(get_position.find(detector_type) != get_position.end())
        get_position.at(detector_type)();

    return direction*position;
}

////////////////////////////////////////////////////////////////////////////////
// clear the stored plane hits

void GEMPlane::ClearStripHits()
{
    strip_hits.clear();
}


////////////////////////////////////////////////////////////////////////////////
// add a plane hit

void GEMPlane::AddStripHit(int strip, float charge, short maxtime, bool xtalk, 
        int crate, int mpd, int adc, const std::vector<float> &_ts_adc)
{
    std::string detector_type = GetDetector() -> GetType();
    if(detector_type == "PRADGEM") {
        // PRad floating strip removal
        // hard coded because it is specific to PRad GEM setting
        if((type == Plane_X) &&
                ((strip < 16) || (strip > 1391)))
            return;
    }

    strip_hits.emplace_back(strip, charge, maxtime, GetStripPosition(strip),
            xtalk, crate, mpd, adc);

    strip_hits.back().ts_adc = _ts_adc;
}


////////////////////////////////////////////////////////////////////////////////
// collect hits from the connected APVs

void GEMPlane::CollectAPVHits()
{
    ClearStripHits();

    for(auto &apv : apv_list)
    {
        if(apv != nullptr)
            apv->CollectZeroSupHits();
    }
}

////////////////////////////////////////////////////////////////////////////////
// form clusters by the clustering method

void GEMPlane::FormClusters(GEMCluster *method)
{
    const std::string &detector_type = detector -> GetType();

    if(detector_type == "HALLDGEM") {
	std::vector<StripHit> strip_hits_odd, strip_hits_even;
	std::vector<StripCluster> strip_clusters_odd, strip_clusters_even;

        // separate odd and even strips, treat them as two different planes
	for(auto &i: strip_hits)
	{
	    StripHit h = i;
	    if(h.strip % 2 == 0) {
		h.strip = h.strip / 2;
		strip_hits_even.push_back(h);
	    }
	    else {
		h.strip = h.strip / 2;
		strip_hits_odd.push_back(h);
	    }
	}

        // group strips to clusters, separately for odd and even strips
	method -> FormClusters(strip_hits_odd, strip_clusters_odd);
	method -> FormClusters(strip_hits_even, strip_clusters_even);

	// merge clusters from even and odd strips
	strip_clusters.clear();
	for(auto &i: strip_clusters_odd)
	    strip_clusters.push_back(i);
	for(auto &i: strip_clusters_even)
	    strip_clusters.push_back(i);
    }
    else {
	method->FormClusters(strip_hits, strip_clusters);
    }
}

void GEMPlane::PrintStatus()
{
    std::cout<<"plane type: "<<GetType()<<" contains "
	<<apv_list.size() << " apvs."<<std::endl;
}
