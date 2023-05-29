#include "standard_align.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <map>

namespace tracking_dev
{
    StandardAlign::StandardAlign()
    {
        tracking_utility = new TrackingUtility();
    }

    StandardAlign::~StandardAlign()
    {
    }

    void StandardAlign::Init()
    {
        if (nparam == 0)
        {
            std::cout << "Please set number of parmaters first." << std::endl;
            exit(0);
        }

        M.SetDimension(nparam, nparam);
        M.Zero();
        b.SetDimension(nparam, 1);
        b.Zero();
        a.SetDimension(nparam, 1);
        a.Zero();
    }

    void StandardAlign::Solve()
    {
        while (MaxIter-- > 0)
        {
            std::cout << "Processing Iteration : " << MaxIter << " ..." << std::endl;
            ProcessIteration();
            SolveIteration();
        }
    }

    void StandardAlign::ProcessIteration()
    {
        CurrentBigChi2 = 0.;
        M.Zero();
        b.Zero();

        std::cout << "Total tracks used for alignment: " << data_cache.size() << std::endl;
        for (auto &i : data_cache)
        {
            ProcessTrack(i);
        }

        std::cout << "Iteration: " << MaxIter << ": previous chi2: " << PrevBigChi2 << " current chi2: " << CurrentBigChi2 << std::endl;
        // chi2 doesn't improve anymore, stop iteration
        double ratio_chi2 = (PrevBigChi2 - CurrentBigChi2) / PrevBigChi2;
        std::cout << "chi2 ratio change = " << ratio_chi2 << std::endl;
        std::cout << "min chi2 ratio change = " << MinDeltaChi2ndf << std::endl;
        if (ratio_chi2 <= MinDeltaChi2ndf)
        {
            std::cout << "chi2 doesn't improve anymore" << std::endl;
            MaxIter = -1;
        }
        PrevBigChi2 = CurrentBigChi2;
    }

    void StandardAlign::ProcessTrack(const std::vector<point_t> &hits)
    {
        // step 1) offset, rotation correction
        std::vector<point_t> corrected_hits;
        Transform(hits, corrected_hits);

        // step 2) local fit
        double xtrack, ytrack, xptrack, yptrack, chi2ndf;
        std::vector<double> xresid, yresid;
        tracking_utility->FitLine(corrected_hits, xtrack, ytrack, xptrack,
                                  yptrack, chi2ndf, xresid, yresid, 0.08, 0.08);
        CurrentBigChi2 += chi2ndf;

        // step 3) update matrix
        int ihit = 0;
        Matrix M_itrack(nparam, nparam), b_itrack(nparam, 1);
        for (auto &i : corrected_hits)
        {
            Matrix Ai, di;
            UpdateMatrixAi(xptrack, yptrack, i, Ai);
            UpdateMatrixdi(xptrack, yptrack, xtrack, ytrack, i, di);

            Matrix Mi, bi;
            Mi = Ai.Transpose() * Ai;
            bi = Ai.Transpose() * di;
            // std::cout<<"M"<<ihit<<std::endl<<Mi<<std::endl;
            // std::cout<<"b"<<ihit<<std::endl<<bi<<std::endl;

            // track
            for (int ii = 0; ii < nparam_per_layer; ii++)
            {
                for (int jj = 0; jj < nparam_per_layer; jj++)
                {
                    M_itrack(ihit * nparam_per_layer + ii, ihit * nparam_per_layer + jj) = Mi.at(ii, jj);
                }

                b_itrack(ihit * nparam_per_layer + ii, 0) = bi.at(ii, 0);
            }
            ihit++;
        }
        // std::cout<<"M_itrack:"<<std::endl<<M_itrack<<std::endl;
        // std::cout<<"b_itrack"<<std::endl<<b_itrack<<std::endl;

        M = M + M_itrack;
        b = b + b_itrack;
    }

    void StandardAlign::SolveIteration()
    {
        // std::cout<<"M matrix: "<<std::endl<<M<<std::endl;
        // std::cout<<"b matrix: "<<std::endl<<b<<std::endl;
        Matrix m_inverse = M.Inverse();
        // M.PrintDimension();
        // getchar();
        Matrix delta_a = m_inverse * b;

        // std::cout << "iteration : " << MaxIter << std::endl;
        // std::cout << "improvement: " << std::endl;
        // std::cout << delta_a << std::endl;

        for (int i = 0; i < nlayer; i++)
        {
            // skip updating anchor layer
            if (m_anchor_layers.find(i) != m_anchor_layers.end())
                continue;

            for (int j = 0; j < nparam_per_layer; j++)
            {
                a(i * nparam_per_layer + j, 0) = a.at(i * nparam_per_layer + j, 0) + delta_a.at(i * nparam_per_layer + j, 0);
            }
        }
        std::cout << "results: " << std::endl;
        std::cout << a << std::endl;
    }

    void StandardAlign::SetNlayer(int n)
    {
        nlayer = n;
        nparam = nlayer * nparam_per_layer;

        Init(); // need re-initialize after setting dimension
    }

    int StandardAlign::GetNparam()
    {
        return nparam;
    }

    std::unordered_map<int, bool> StandardAlign::GetAnchorLayers()
    {
        return m_anchor_layers;
    }

    void StandardAlign::WriteTextFile(const char *path)
    {
        std::fstream f(path, std::fstream::out);
        if (!f.is_open())
        {
            std::cout << "Error: can't open file: " << path << std::endl;
            exit(0);
        }

        std::cout << "INFO:: Alignment: Writing results to :" << path << std::endl;
        f << a;
        f.close();
    }

    void StandardAlign::LoadTextFile(const char *path)
    {
        std::fstream f(path, std::fstream::in);
        if (!f.is_open())
        {
            std::cout << "Error: cannot open file: " << path << std::endl;
            exit(0);
        }
        std::vector<double> cache;
        std::string line;
        while (std::getline(f, line))
        {
            if (line[line.size() - 1] == ',')
                line = line.substr(0, line.size() - 1);

            try
            {
                double tmp = std::stod(line);
                cache.push_back(tmp);
                std::cout << tmp << std::endl;
            }
            catch (...)
            {
                std::cout << "ERROR:: failed to convert string to double" << std::endl;
            }
        }

        size_t N = cache.size();
        a.SetDimension(N, 1);
        for (size_t i = 0; i < N; i++)
            a(i, 0) = cache[i];
        std::cout << "INFO:: Alignment: started with parameter: " << std::endl
                  << a << std::endl;
    }

    void StandardAlign::SetupToyModel()
    {
        toy = new ToyModel();
        toy->Load();

        CopyToyModelData();
    }

    void StandardAlign::CopyToyModelData()
    {
        const auto &all_events = toy->GetAllEvents();
        std::map<double, int> _tmp_index;
        for (auto &i : all_events)
        {
            std::vector<point_t> tmp;
            for (auto &hit : i.hits)
            {
                tmp.push_back(hit);
                _tmp_index[hit.z] = 0;
            }

            Sort(tmp);
            data_cache.push_back(tmp);
        }

        // create unordered_maps to correlate layer_id vs layer_z
        int layer_id = 0;
        for (auto &i : _tmp_index)
        {
            index_to_z[layer_id] = i.first;
            z_to_index[i.first] = layer_id;
            layer_id++;
        }
        // for(auto &i: index_to_z) {
        //     std::cout<<"layer id = "<<i.first<<" layer z = "<<i.second<<std::endl;
        // }
    }

    // sort hits, ascending order in z
    void StandardAlign::Sort(std::vector<point_t> &hits)
    {
        std::sort(hits.begin(), hits.end(), [&](const point_t &h1, const point_t &h2)
                  {
                if(h1.z < h2.z) return true;
                return false; });
    }

    void StandardAlign::Transform(const std::vector<point_t> &in, std::vector<point_t> &out)
    {
        for (auto &i : in)
        {
            out.push_back(Transform(i));
        }
    }

    point_t StandardAlign::Transform(const point_t &p)
    {
        if (z_to_index.find(p.z) == z_to_index.end())
        {
            std::cout << "Error: found no layer id for point: " << p << std::endl;
            exit(0);
        }

        int ilayer = z_to_index[p.z];
        double dx = a.at(nparam_per_layer * ilayer, 0);
        double dy = a.at(nparam_per_layer * ilayer + 1, 0);
        double dz = a.at(nparam_per_layer * ilayer + 2, 0);
        double ax = a.at(nparam_per_layer * ilayer + 3, 0);
        double ay = a.at(nparam_per_layer * ilayer + 4, 0);
        double az = a.at(nparam_per_layer * ilayer + 5, 0);

        point_t res;
        res.x = p.x - az * p.y + ay * p.z + dx;
        res.y = az * p.x + p.y - ax * p.z + dy;
        res.z = -ay * p.x + ax * p.y + p.z + dz;

        return res;
    }

    void StandardAlign::UpdateMatrixAi(const double &kx, const double &ky,
                                       const point_t &pi, Matrix &Ai)
    {
        Ai.SetDimension(2, nparam_per_layer);

        Ai(0, 0) = 1, Ai(0, 1) = 0, Ai(0, 2) = -kx;
        Ai(0, 3) = -pi.y * kx, Ai(0, 4) = pi.z + pi.x * kx, Ai(0, 5) = -pi.y;
        // Ai(0, 3) = -pi.y * kx, Ai(0, 4) = pi.x*kx, Ai(0, 5) = -pi.y; // incorrect

        Ai(1, 0) = 0, Ai(1, 1) = 1, Ai(1, 2) = -ky;
        Ai(1, 3) = -(pi.z + pi.y * ky), Ai(1, 4) = pi.x * ky, Ai(1, 5) = pi.x;
        // Ai(1, 3) = -(pi.y * ky), Ai(1, 4) = pi.x*ky, Ai(1, 5) = pi.x; // incorrect
    }

    void StandardAlign::UpdateMatrixdi(const double &kx, const double &ky,
                                       const double &bx, const double &by,
                                       const point_t &pi, Matrix &di)
    {
        di.SetDimension(2, 1);

        di(0, 0) = kx * pi.z + bx - pi.x;
        di(1, 0) = ky * pi.z + by - pi.y;
    }

};
