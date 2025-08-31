void cluster_tree_analyze_example()
{
    TFile *f = new TFile("../Rootfiles/cluster_bbgem_388.root");
    TTree *T = (TTree*)f->Get("GEMCluster");

    // setup tree branch
    const int MaxClusterPerEvent = 10000;

    int eventID;
    int nCluster;
    int Plane[MaxClusterPerEvent];
    int Prod[MaxClusterPerEvent];
    int Module[MaxClusterPerEvent];
    int Axis[MaxClusterPerEvent];
    int Size[MaxClusterPerEvent];
    float ADC[MaxClusterPerEvent];
    float Position[MaxClusterPerEvent];

    int StripNo[MaxClusterPerEvent][100];
    float StripADC[MaxClusterPerEvent][100];

    T->SetBranchAddress("evtID", &eventID);
    T->SetBranchAddress("nCluster", &nCluster);
    T->SetBranchAddress("planeID", Plane);
    T->SetBranchAddress("prodID", Prod);
    T->SetBranchAddress("moduleID", Module);
    T->SetBranchAddress("axis", Axis);
    T->SetBranchAddress("size", Size);
    T->SetBranchAddress("adc", ADC);
    T->SetBranchAddress("pos", Position);
    T->SetBranchAddress("stripNo", StripNo);
    T->SetBranchAddress("stripAdc", StripADC);

    // analyze each event
    int entries = T->GetEntries();
    cout<<"total entries: "<<entries<<endl;

    for(int entry=0; entry<entries; ++entry)
    {
        // get one event 
        T -> GetEntry(entry);

        cout<<"event id: "<<eventID<<endl;
        cout<<"total number of clusters in current event: "<<nCluster<<endl;

        for(int nC=0; nC<nCluster; ++nC)
        {
            cout<<"------------------------------------------------------"<<endl;
            cout<<"information for cluster "<<nC<<":"<<endl;
            cout<<setfill(' ')<<setw(15)<<"layer_id"
                <<setfill(' ')<<setw(15)<<"GEM_ID"
                <<setfill(' ')<<setw(15)<<"GEM_POS"
                <<setfill(' ')<<setw(15)<<"Axis(x/y)"
                <<setfill(' ')<<setw(15)<<"cluster_size"
                <<setfill(' ')<<setw(15)<<"charge"
                <<setfill(' ')<<setw(15)<<"position"
                <<endl;

            cout<<setfill(' ')<<setw(15)<<Plane[nC]
                <<setfill(' ')<<setw(15)<<Prod[nC]
                <<setfill(' ')<<setw(15)<<Module[nC]
                <<setfill(' ')<<setw(15)<<Axis[nC]
                <<setfill(' ')<<setw(15)<<Size[nC]
                <<setfill(' ')<<setw(15)<<ADC[nC]
                <<setfill(' ')<<setw(15)<<Position[nC]
                <<endl;


            int numberOfStripsInThisCluster = Size[nC];
            cout<<"strip index in this cluster:"<<endl;
            for(int strip=0; strip<numberOfStripsInThisCluster; ++strip){
                cout<<setfill(' ')<<setw(9)<<StripNo[nC][strip];
            }
            cout<<endl;

            cout<<"strip charge in this cluster:"<<endl;
            for(int strip=0; strip<numberOfStripsInThisCluster; ++strip){
                cout<<setfill(' ')<<setw(9)<<StripADC[nC][strip];
            }
            cout<<endl;
        }

        cout<<"press Enter to check next event..."<<endl;
        getchar();
    }
}
