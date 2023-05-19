#ifndef GEM_DETECTOR_H
#define GEM_DETECTOR_H

#include <string>
#include <vector>
#include "GEMStruct.h"
#include "GEMPlane.h"

// reserve space for faster filling of clusters
#define GEM_CLUSTERS_BUFFER 500

class GEMSystem;
class GEMDetectorLayer;
class GEMCluster;
class GEMAPV;

class GEMDetector
{
public:
    friend class GEMCluster;

public:
    // constructor
    GEMDetector(const std::string &readoutBoard,
                const std::string &detectorType,
                const std::string &detectorName,
                const int &detectorID,
                const int &layerID,
                const int &layer_index,
                GEMSystem *g = nullptr);

    // copy/move constructors
    GEMDetector(const GEMDetector &that);
    GEMDetector(GEMDetector &&that);

    // desctructor
    virtual ~GEMDetector();

    // copy/move assignment operators
    GEMDetector &operator =(const GEMDetector &rhs);
    GEMDetector &operator =(GEMDetector &&rhs);

    // public member functions
    void SetSystem(GEMSystem *sys, bool false_set = false);
    void SetGEMLayer(GEMDetectorLayer *l);
    void SetResolution(double r) {res = r;}
    void SetID(int i){det_id = i;}
    void setLayerID(int i){layer_id = i;}
    void SetLayerPositionIndex(int i){layer_position_index = i;}
    void UnsetSystem(bool false_unset = false);
    bool AddPlane(GEMPlane *plane);
    bool AddPlane(const int &type, const std::string &name, const double &size,
                  const int &conn, const int &ori, const int &dir);
    void RemovePlane(const int &type);
    void DisconnectPlane(const int &type, bool false_disconn = false);
    void ConnectPlanes();
    void Reconstruct(GEMCluster *c);
    void CollectHits();
    void ClearHits();
    void Reset();

    // get parameters
    GEMSystem *GetSystem() const {return gem_sys;}
    GEMDetectorLayer *GetLayer() const {return gem_layer;}
    double GetResolution() const {return res;}
    const std::string &GetType() const {return type;}
    const std::string &GetReadoutBoard() const {return readout_board;}
    GEMPlane *GetPlane(const int &type) const;
    GEMPlane *GetPlane(const std::string &type) const;
    std::vector<GEMPlane*> GetPlaneList() const;
    std::vector<GEMAPV*> GetAPVList(const int &type) const;
    std::vector<GEMHit> &GetHits() {return gem_hits;}
    const std::vector<GEMHit> &GetHits() const {return gem_hits;}
    int GetDetID() const {return det_id;}
    int GetLayerID() const {return layer_id;}
    int GetDetLayerPositionIndex() const {return layer_position_index;}
    const std::string &GetName() const {return det_name;}
    void PrintStatus();

private:
    GEMSystem *gem_sys;
    GEMDetectorLayer *gem_layer;
    std::string det_name;
    int det_id = -1;
    int layer_id;
    int layer_position_index;
    std::string type;
    std::string readout_board;
    std::vector<GEMPlane*> planes;
    std::vector<GEMHit> gem_hits;
    float res;
};

#endif
