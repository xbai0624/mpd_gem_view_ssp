#ifndef GEM_ROOT_HIT_TREE_H
#define GEM_ROOT_HIT_TREE_H

#include <TTree.h>
#include <TFile.h>

#include "GEMStruct.h"
#include "GEMSystem.h"

////////////////////////////////////////////////////////////////////////////////
// save replayed evio files to root tree

#define MAXHITS 20000

class GEMRootHitTree
{
public:
    GEMRootHitTree(const char* path);
    ~GEMRootHitTree();

    void Write();
    void Fill(GEMSystem *gem_sys, const EventData &ev);

private:
    TTree *pTree = nullptr;
    TFile *pFile = nullptr;

    std::string fPath;

    // information to save
    int evtID;
    int nch;
    int Plane[MAXHITS];    // layer id
    int Prod[MAXHITS];     // gem id (production id given by UVa)
    int Module[MAXHITS];   // gem location in layer
    int Strip[MAXHITS];    // strip index on a single chamber
    int Axis[MAXHITS];  // x or y plane

    int adc0[MAXHITS];
    int adc1[MAXHITS];
    int adc2[MAXHITS];
    int adc3[MAXHITS];
    int adc4[MAXHITS];
    int adc5[MAXHITS];
    int adc6[MAXHITS];
    int adc7[MAXHITS];
    int adc8[MAXHITS];

    int triggerTimeL;
    int triggerTimeH;
};

#endif
