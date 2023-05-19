#ifndef GEM_PLANE_H
#define GEM_PLANE_H

#include <cstdint>
#include "GEMAPV.h"
#include "ConfigParser.h"

class GEMDetector;
class GEMCluster;

class GEMPlane
{
public:
    enum Type
    {
        Undefined_Type = -1,
        Plane_X = 0,
        Plane_Y,
        Max_Types,
    };
    // macro in ConfigParser.h
    ENUM_MAP(Type, 0, "X|Y");

public:
    // constructors
    GEMPlane(GEMDetector *det = nullptr);
    GEMPlane(const std::string &n, const int &t, const float &s, const int &c,
            const int &o, const int &d, GEMDetector *det = nullptr);

    // copy/move constructors
    GEMPlane(const GEMPlane &that);
    GEMPlane(GEMPlane &&that);

    // destructor
    virtual ~GEMPlane();

    // copy/move assignment operators
    GEMPlane &operator= (const GEMPlane &rhs);
    GEMPlane &operator= (GEMPlane &&rhs);

    // public member functions
    void ConnectAPV(GEMAPV *apv, const int &index);
    void DisconnectAPV(const uint32_t &plane_index, bool force_disconn);
    void DisconnectAPVs();
    void AddStripHit(int strip, float charge, short timebin, bool xtalk, int crate, int mpd, int adc, const std::vector<float> &ts_adc);
    void ClearStripHits();
    void CollectAPVHits();
    float GetStripPosition(const int &plane_strip) const;
    void FormClusters(GEMCluster *method);

    // set parameter
    void SetDetector(GEMDetector *det, bool force_set = false);
    void UnsetDetector(bool force_unset = false);
    void SetName(const std::string &n) {name = n;}
    void SetType(const Type &t) {type = t;}
    void SetSize(const float &s) {size = s;}
    void SetOrientation(const int &o) {orient = o;}
    void SetCapacity(int c);

    // get parameter
    GEMDetector *GetDetector() const {return detector;}
    const std::string &GetName() const {return name;}
    Type GetType() const {return type;}
    float GetSize() const {return size;}
    int GetCapacity() const {return apv_list.size();}
    int GetOrientation() const {return orient;}
    std::vector<GEMAPV*> GetAPVList() const;
    std::vector<StripHit> &GetStripHits() {return strip_hits;}
    const std::vector<StripHit> &GetStripHits() const {return strip_hits;}
    std::vector<StripCluster> &GetStripClusters() {return strip_clusters;}
    const std::vector<StripCluster> &GetStripClusters() const {return strip_clusters;};
    void PrintStatus();

private:
    GEMDetector *detector;
    std::string name;
    Type type;
    float size;
    int orient;
    int direction;
    std::vector<GEMAPV*> apv_list;

    // plane raw hits and clusters
    std::vector<StripHit> strip_hits;
    std::vector<StripCluster> strip_clusters;
};

#endif
