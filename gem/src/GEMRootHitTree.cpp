#include "GEMRootHitTree.h"
#include "APVStripMapping.h"
#include "PreAnalysis.h"

////////////////////////////////////////////////////////////////////////////////
// ctor

GEMRootHitTree::GEMRootHitTree(const char* path)
{
    fPath = path;
    pFile = new TFile(path,"RECREATE");
    pTree = new TTree("GEMHit","Hit list");

    pTree->Branch("evtID",&evtID,"evtID/I");
    pTree->Branch("nch", &nch, "nch/I");
    pTree->Branch("planeID",Plane,"planeID[nch]/I");
    pTree->Branch("prodID",Prod,"prodID[nch]/I");
    pTree->Branch("moduleID",Module,"moduleID[nch]/I");
    pTree->Branch("axis",Axis,"axis[nch]/I");
    pTree->Branch("strip",Strip,"strip[nch]/I");

    pTree->Branch("adc0",adc0,"adc0[nch]/I");
    pTree->Branch("adc1",adc1,"adc1[nch]/I");
    pTree->Branch("adc2",adc2,"adc2[nch]/I");

    pTree->Branch("adc3",adc3,"adc3[nch]/I");
    pTree->Branch("adc4",adc4,"adc4[nch]/I");
    pTree->Branch("adc5",adc5,"adc5[nch]/I");

    pTree->Branch("adc6",adc3,"adc6[nch]/I");
    pTree->Branch("adc7",adc4,"adc7[nch]/I");
    pTree->Branch("adc8",adc5,"adc8[nch]/I");


    pTree->Branch("triggerTimeL", &triggerTimeL, "triggerTimeL/I");
    pTree->Branch("triggerTimeH", &triggerTimeH, "triggerTimeH/I");
}

////////////////////////////////////////////////////////////////////////////////
// dtor

GEMRootHitTree::~GEMRootHitTree()
{
    // when TFile closes (in Write() function), 
    // it also deletes the tree associated,
    // no need to delete twice

    //delete pTree;
    //delete pFile;
}

////////////////////////////////////////////////////////////////////////////////
// write()

void GEMRootHitTree::Write()
{
    std::cout<<"writing root file to: "<<fPath<<std::endl;
    //pTree->Write();
    pFile->Write();
    pFile->Close();

    PreAnalysis::Instance()->SavePlots();
}

////////////////////////////////////////////////////////////////////////////////
// Fill event
// gem_sys: a needed helper, to get detector configuration informationi
// ev: event data to be filled

void GEMRootHitTree::Fill(GEMSystem *gem_sys, const EventData &ev)
{
    const std::vector<GEM_Strip_Data> &strip_data = ev.get_gem_data();
    evtID = ev.event_number;
    nch = strip_data.size();

    std::pair<uint32_t, uint32_t> trigger_time = gem_sys -> GetTriggerTime();
    //std::cout<<trigger_time.first<<", "<<trigger_time.second<<std::endl;
    triggerTimeL = static_cast<int>(trigger_time.first);
    triggerTimeH = static_cast<int>(trigger_time.second);
   
    // only save 20000 hits
    if(nch > MAXHITS)
        nch = MAXHITS;

    for(int i=0;i<nch;i++)
    {
        adc0[i] = static_cast<int>(strip_data[i].values[0]);
        adc1[i] = static_cast<int>(strip_data[i].values[1]);
        adc2[i] = static_cast<int>(strip_data[i].values[2]);
        adc3[i] = static_cast<int>(strip_data[i].values[3]);
        adc4[i] = static_cast<int>(strip_data[i].values[4]);
        adc5[i] = static_cast<int>(strip_data[i].values[5]);
        adc6[i] = static_cast<int>(strip_data[i].values[6]);
        adc7[i] = static_cast<int>(strip_data[i].values[7]);
        adc8[i] = static_cast<int>(strip_data[i].values[8]);

        GEMChannelAddress addr = strip_data[i].addr;

        Plane[i] = apv_strip_mapping::Mapping::Instance()->GetPlaneID(addr);
        Prod[i] = apv_strip_mapping::Mapping::Instance()->GetProdID(addr);
        Module[i] = apv_strip_mapping::Mapping::Instance()->GetModuleID(addr);

        Axis[i] = apv_strip_mapping::Mapping::Instance()->GetAxis(addr);
        if(Axis[i] != 0 && Axis[i] != 1)
            std::cout<<"Error: "<<Axis[i]<<std::endl;

        const std::string & detector_type = gem_sys -> GetAPV(addr.crate, addr.mpd, addr.adc)
            -> GetPlane() -> GetDetector() -> GetType();
        Strip[i] = apv_strip_mapping::Mapping::Instance()->GetStrip(detector_type, addr);
    }
    
    if(pTree != nullptr) {
        if(nch > 0)
            pTree->Fill();
    }
    else {
        std::cout<<__func__<<" Error: root tree is nullptr."<<std::endl;
    }

    // for thir
    PreAnalysis::Instance()->UpdateEvent(ev);
}
