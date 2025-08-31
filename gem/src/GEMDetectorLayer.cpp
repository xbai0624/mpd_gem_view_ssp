////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Detector Layer contains multiple GEM detectors                             //
// For SBS configuration, one layer contains n GEM detectors                  //
//                                                                            //
// Xinzhan Bai                                                                //
// 03/08/2021                                                                 //
////////////////////////////////////////////////////////////////////////////////

#include "GEMDetectorLayer.h"

GEMDetectorLayer::GEMDetectorLayer(
        const int &layer_id, const int &nChamber, 
        const std::string& _readout_type,
        const double &_x_offset, const double &_y_offset,
        const std::string &gem_type,
        const int &nAPVsGEMX, const int &nAPVsGEMY,
        const double &_x_pitch, const double &_y_pitch,
        const int &_x_flip, const int &_y_flip
        )
    : id(layer_id), nChambersPerLayer(nChamber), readout_type(_readout_type),
    x_offset(_x_offset), y_offset(_y_offset), gem_type(gem_type),
    nb_apvs_chamber_x_plane(nAPVsGEMX), nb_apvs_chamber_y_plane(nAPVsGEMY),
    x_pitch(_x_pitch), y_pitch(_y_pitch), x_flip(_x_flip), y_flip(_y_flip)
{
    fDetectorList.clear();
}

GEMDetectorLayer::GEMDetectorLayer() : id(-1), readout_type("CARTESIAN"),
    x_offset(0), y_offset(0),
    gem_type("UVAGEM"), nb_apvs_chamber_x_plane(-1),
    nb_apvs_chamber_y_plane(-1), x_flip(1), y_flip(1)
{
    fDetectorList.clear();
}

GEMDetectorLayer::~GEMDetectorLayer()
{
    // place holder
}

void GEMDetectorLayer::AddGEMDetector(GEMDetector* det)
{
    for(auto &i: fDetectorList)
        if(i == det)
            return;

    fDetectorList.push_back(det);
}

const std::vector<GEMDetector*> & GEMDetectorLayer::GetDetectorList() 
    const
{
    return fDetectorList;
}

int GEMDetectorLayer::GetNumberOfDetectorsInLayer() const
{
    int res = static_cast<int>(fDetectorList.size());
    if(res != nChambersPerLayer) {
        std::cout<<__func__<<" Error: number of detector in layer: "<<res
                 <<" not consistent with configuration: "<<nChambersPerLayer
                 <<" **check mapping**"
                 <<std::endl;
        exit(0);
    }
    return res;
}

std::string GEMDetectorLayer::GetGEMChamberType() const
{
    return gem_type;
}

std::string GEMDetectorLayer::GetReadoutType() const
{
    return readout_type;
}

const double& GEMDetectorLayer::GetXOffset() const
{
    return x_offset;
}

const double& GEMDetectorLayer::GetYOffset() const
{
    return y_offset;
}

int GEMDetectorLayer::GetNumberOfAPVsOnChamberXPlane() const
{
    return nb_apvs_chamber_x_plane;
}

int GEMDetectorLayer::GetNumberOfAPVsOnChamberYPlane() const
{
    return nb_apvs_chamber_y_plane;
}

int GEMDetectorLayer::GetNumberOfAPVsOnChamberPlane(const int &type)
    const 
{
    if(type == 0)
        return GetNumberOfAPVsOnChamberXPlane();
    else if(type == 1)
        return GetNumberOfAPVsOnChamberYPlane();
    else {
        std::cout<<__func__<<" Error: GEM Plane type :"<<type<<" not supported."
                 <<std::endl;
        return 99;
    }
}

double GEMDetectorLayer::GetChamberPlanePitch(const int &type)
    const
{
    if(type == 0)
        return GetChamberXPitch();
    else if(type == 1)
        return GetChamberYPitch();
    else {
        std::cout<<__func__<<" Error: GEM Plane type :"<<type<<" not supported."
                 <<std::endl;
        return 0.4;
    }
}

int GEMDetectorLayer::GetFlipByPlaneType(const int &type)
    const
{
    if(type == 0)
        return GetXFlip();
    else if(type == 1)
        return GetYFlip();
    else {
        std::cout<<__func__<<" Error: GEM Plane type :"<<type<<" not supported."
                 <<std::endl;
        return 1;
    }
}

void GEMDetectorLayer::PrintStatus()
{
    std::cout<<"Layer Id = "<<GetID()<<" contains "
        <<fDetectorList.size()<<" detectors."<<std::endl;
    for(auto &i: fDetectorList) {
        i -> PrintStatus();
    }
}
