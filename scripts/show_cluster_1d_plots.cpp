
unordered_map<int, pair<TH1F*, TH1F*>> m_strip_pos;
unordered_map<int, pair<TH1F*, TH1F*>> m_cluster_pos;
unordered_map<int, pair<TH1F*, TH1F*>> m_cluster_num;
unordered_map<int, pair<TH1F*, TH1F*>> m_cluster_size;
unordered_map<int, pair<TH1F*, TH1F*>> m_cluster_adc;

void init_histo_for_det(int id)
{
    cout<<"initing histos for detector: "<<id<<endl;
    TH1F* strip_pos_x = new TH1F(Form("h_det%d_x_strip_pos", id), Form("h_det%d_x_strip_pos", id), 5000, 0, 5000);
    TH1F* strip_pos_y = new TH1F(Form("h_det%d_y_strip_pos", id), Form("h_det%d_y_strip_pos", id), 5000, 0, 5000);
    m_strip_pos[id] = pair<TH1F*, TH1F*>(strip_pos_x, strip_pos_y);

    TH1F* cluster_pos_x = new TH1F(Form("h_det%d_x_cluster_pos", id), Form("h_det%d_x_cluster_pos", id), 5000, 0, 5000);
    TH1F* cluster_pos_y = new TH1F(Form("h_det%d_y_cluster_pos", id), Form("h_det%d_y_cluster_pos", id), 5000, 0, 5000);
    m_cluster_pos[id] = pair<TH1F*, TH1F*>(cluster_pos_x, cluster_pos_y);

    TH1F* cluster_num_x = new TH1F(Form("h_det%d_x_cluster_num", id), Form("h_det%d_x_cluster_num", id), 1000, 0, 1000);
    TH1F* cluster_num_y = new TH1F(Form("h_det%d_y_cluster_num", id), Form("h_det%d_y_cluster_num", id), 1000, 0, 1000);
    m_cluster_num[id] = pair<TH1F*, TH1F*>(cluster_num_x, cluster_num_y);

    TH1F* cluster_size_x = new TH1F(Form("h_det%d_x_cluster_size", id), Form("h_det%d_x_cluster_size", id), 100, 0, 100);
    TH1F* cluster_size_y = new TH1F(Form("h_det%d_y_cluster_size", id), Form("h_det%d_y_cluster_size", id), 100, 0, 100);
    m_cluster_size[id] = pair<TH1F*, TH1F*>(cluster_size_x, cluster_size_y);

    TH1F* cluster_adc_x = new TH1F(Form("h_det%d_x_cluster_adc", id), Form("h_det%d_x_cluster_adc", id), 2000, 0, 2000);
    TH1F* cluster_adc_y = new TH1F(Form("h_det%d_y_cluster_adc", id), Form("h_det%d_y_cluster_adc", id), 2000, 0, 2000);
    m_cluster_adc[id] = pair<TH1F*, TH1F*>(cluster_adc_x, cluster_adc_y);
}

void save_histos(const char* path)
{
    cout<<"saving results to : "<<path<<endl;
    TFile *f = new TFile(path, "recreate");
    for(auto &i: m_strip_pos) {
        cout<<"stip position total entries: "<<i.second.first -> GetEntries()<<endl;
        i.second.first->Write();
        i.second.second -> Write();
    }
    for(auto &i: m_cluster_pos) {
        cout<<"cluster position total entries: "<<i.second.first -> GetEntries()<<endl;
        i.second.first->Write();
        i.second.second -> Write();
    }
    for(auto &i: m_cluster_num) {
        cout<<"cluster quantity total entries: "<<i.second.first -> GetEntries()<<endl;
        i.second.first->Write();
        i.second.second -> Write();
    }
    for(auto &i: m_cluster_size) {
        cout<<"cluster size total entries: "<<i.second.first -> GetEntries()<<endl;
        i.second.first->Write();
        i.second.second -> Write();
    }
    for(auto &i: m_cluster_adc) {
        cout<<"cluster adc total entries: "<<i.second.first -> GetEntries()<<endl;
        i.second.first->Write();
        i.second.second -> Write();
    }
    f->Close();
}

#define MAX 10000

const int det_id[] = {
    3, 13, 17, 20, 25
};

void show_cluster_1d_plots(const char* path="../Rootfiles/test.root")
{
    int evtID;
    int nCluster;

    int layerID[MAX];
    int detID[MAX];
    int layerPosIndex[MAX];
    int plane[MAX];
    int size[MAX];
    float adc[MAX];
    float pos[MAX];

    int stripNo[MAX][100];
    float stripADC[MAX][100];

    TFile *f = new TFile(path);
    TTree *T = (TTree*)f->Get("GEMCluster");

    T->SetBranchAddress("evtID", &evtID);
    T->SetBranchAddress("nCluster", &nCluster);
    T->SetBranchAddress("planeID", layerID);
    T->SetBranchAddress("prodID", detID);
    T->SetBranchAddress("moduleID", layerPosIndex);
    T->SetBranchAddress("axis", plane);
    T->SetBranchAddress("size", size);
    T->SetBranchAddress("adc", adc);
    T->SetBranchAddress("pos", pos);
    T->SetBranchAddress("stripNo", stripNo);
    T->SetBranchAddress("stripAdc", stripADC);

    int entries = T->GetEntries();
    cout<<"total entries: "<<entries<<endl;

    // init all histos
    for(auto &i: det_id)
        init_histo_for_det(i);

    for(int entry = 0; entry<entries; ++entry)
    {
        T -> GetEntry(entry);

        for(int nc =0; nc<nCluster; ++nc)
        {
            int detector_ID = detID[nc];
            if(m_cluster_size.find(detector_ID) == m_cluster_size.end()) {
                cout<<"Error: detector id: "<<detector_ID<<", is not initialized."<<endl;
                continue;
            }
            int axis = plane[nc];
            switch(axis) {
                case 0:
                    {
                        m_cluster_size[detector_ID].first -> Fill(size[nc]);
                        for(int i=0; i<size[nc]; ++i) {
                            m_strip_pos[detector_ID].first -> Fill(stripNo[nc][i]);
                        }
                        m_cluster_pos[detector_ID].first -> Fill(pos[nc]);
                        m_cluster_adc[detector_ID].first -> Fill(adc[nc]);
                        break;
                    }
                case 1:
                    {
                        m_cluster_size[detector_ID].second -> Fill(size[nc]);
                        for(int i=0; i<size[nc]; ++i) {
                            m_strip_pos[detector_ID].second -> Fill(stripNo[nc][i]);
                        }
                        m_cluster_pos[detector_ID].second -> Fill(pos[nc]);
                        m_cluster_adc[detector_ID].second -> Fill(adc[nc]);
                        break;
                    }
                default:
                    cout<<"unsupported axis: "<<axis<<endl;
                    break;
            }
        }
    }

    save_histos("tmp_plots/test_results.root");
}
