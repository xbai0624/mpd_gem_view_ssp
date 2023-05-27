#ifndef STANDARD_ALIGN_H
#define STANDARD_ALIGN_H

#include "tracking_struct.h"
#include "TrackingUtility.h"
#include "matrix.h"
#include "ToyModel.h"
#include <unordered_map>

namespace tracking_dev {
    class StandardAlign 
    {
    public:
        StandardAlign();
        ~StandardAlign();

        void Init();
        void WriteTextFile(const char* path);
        void LoadTextFile(const char* path);
        void SetupToyModel();
        void CopyToyModelData();
        void Solve();
        void ProcessIteration();
        void ProcessTrack(const std::vector<point_t> &hits);
        void Sort(std::vector<point_t> &hits);
        void Transform(const std::vector<point_t> &in, std::vector<point_t> &out);
        point_t Transform(const point_t &p);
        void UpdateMatrixAi(const double &kx, const double &ky,
                const point_t &pi, Matrix &Ai);
        void UpdateMatrixdi(const double &kx, const double &ky,
                const double &bx, const double &by,
                const point_t &pi, Matrix &di);
        void SolveIteration();

        void SetNlayer(int n);
        template<typename T=int, typename... Args> void SetAnchorLayers(T l, Args... args)
        {
            m_anchor_layers[l] = true;
            SetAnchorLayers(args...);
        }
        void SetAnchorLayers(){}
        int GetNparam();
        std::unordered_map<int, bool> GetAnchorLayers();

    private:
        int MaxIter = 100;
        double MinDeltaChi2ndf = 1e-6;
        double PrevBigChi2 = 1e9;
        double CurrentBigChi2 = 0;
        int nlayer = 0;
        int nparam = 0;
        int nparam_per_layer = 6;
        std::unordered_map<int, bool> m_anchor_layers;

        Matrix M;
        Matrix b;
        Matrix a;

        // indexing layers according to z position
        std::unordered_map<double, int> z_to_index;
        std::unordered_map<int, double> index_to_z;

        // data cache
        std::vector<std::vector<point_t>> data_cache;
        //
        TrackingUtility *tracking_utility;

        // 
        ToyModel *toy;
    };

};

#endif
