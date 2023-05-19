#ifndef GEMDetectorLayer_H
#define GEMDetectorLayer_H

#include "GEMDetector.h"
#include "GEMSystem.h"
#include <vector>

class GEMDetectorLayer 
{
public:
    GEMDetectorLayer(const int &layer_id, const int &nChamber,
                     const std::string& readout_type,
                     const double &x_offset, const double &y_offset,
                     const std::string &gem_type,
                     const int &nAPVsGEMX, const int &nAPVsGEMY,
                     const double &x_pitch, const double &y_pitch,
                     const int &x_flip, const int &y_flip);
    // default constructor
    GEMDetectorLayer();
    // copy/move constructors
    GEMDetectorLayer(const GEMDetectorLayer &that) = default;
    GEMDetectorLayer(GEMDetectorLayer &&that) = default;
    ~GEMDetectorLayer();

    // copy/move assignment
    GEMDetectorLayer &operator =(const GEMDetectorLayer &rhs) = default;
    GEMDetectorLayer &operator =(GEMDetectorLayer &&rhs) = default;

    void AddGEMDetector(GEMDetector* det);

    // setters
    void SetID(int i) {id = i;}
    void SetGEMChamberType(const std::string &t) {gem_type = t;}
    void SetReadoutType(const std::string &t){readout_type = t;}
    void SetXOffset(const double &o){x_offset = o;}
    void SetYOffset(const double &o){y_offset = o;}
    void SetNumberOfAPVsOnChamberXPlane(const int &n){
        nb_apvs_chamber_x_plane = n;
    }
    void SetNumberOfAPVsOnChamberYPlane(const int &n){
        nb_apvs_chamber_y_plane = n;
    }
    void SetSystem(GEMSystem *sys) {gem_sys = sys;}

    // getters
    const std::vector<GEMDetector*> & GetDetectorList() const;
    int GetID() const {return id;}
    int GetNumberOfDetectorsInLayer() const;
    std::string GetGEMChamberType() const;
    std::string GetReadoutType() const;
    const double & GetXOffset() const;
    const double & GetYOffset() const;
    int GetNumberOfAPVsOnChamberXPlane() const;
    int GetNumberOfAPVsOnChamberYPlane() const;
    int GetNumberOfAPVsOnChamberPlane(const int &type) const;
    double GetChamberXPitch() const {return x_pitch;}
    double GetChamberYPitch() const {return y_pitch;}
    int GetXFlip() const {return x_flip;}
    int GetYFlip() const {return y_flip;}
    int GetFlipByPlaneType(const int &type) const;
    double GetChamberPlanePitch(const int &type) const;
    GEMSystem *GetSystem() {return gem_sys;}
    void PrintStatus();

private:
    int id;
    int nChambersPerLayer;
    std::string readout_type;
    double x_offset;
    double y_offset;
    std::string gem_type;
    int nb_apvs_chamber_x_plane;
    int nb_apvs_chamber_y_plane;
    double x_pitch;
    double y_pitch;
    int x_flip; // 1 means no flip, -1 means flip
    int y_flip; // 1 means no flip, -1 means flip

    std::vector<GEMDetector*> fDetectorList;

    GEMSystem *gem_sys;
};

#endif
