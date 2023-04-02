
// a helper
TH1F* plot_apv(const vector<float> &v, int crate, int mpd, int adc, const char* prefix="noise")
{
    if(v.size() != 128) {
        cout<<"Error reading apv data."<<endl;
        return nullptr;
    }

    TH1F *h = new TH1F(Form("%s_crate_%d_mpd_%d_adc_%d", prefix, crate, mpd, adc),
            Form("%s_crate_%d_mpd_%d_adc_%d", prefix, crate, mpd, adc),
            138, -5, 133);

    for(size_t i=0;i<v.size();i++)
    {
        h->SetBinContent(i+5, v[i]);
    }

    return h;
}

// divide historgrams in canvas
void draw()
{
    int nHistoPerCanvas = 20;
    int nCanvas = 440/20;
    TCanvas *c[nCanvas];
    for(int i=0;i<nCanvas;i++) {
        c[i] = new TCanvas(Form("c%d",i), Form("c%d", i), 1000, 800);
        c[i] -> Divide(5, 4);
    }

    TFile *f = new TFile("pedestal.root");
    TIter keyList(f->GetListOfKeys());
    TKey *key;
    int count = 0;
    while( (key = (TKey*)keyList()) ) 
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl -> InheritsFrom("TH1")) continue;
        TH1F * h = (TH1F*)key->ReadObj();

        string name = h->GetName();
        if(name.find("noise") == string::npos) continue;

        int nc = count / nHistoPerCanvas;
        int nci = count % nHistoPerCanvas;

        c[nc] -> cd(nci + 1);
        h->Draw();

        count++;
    }

    for(int i=0;i<nCanvas;i++)
        c[i] -> Draw();
}

// plot pedestal
void plot_pedestal()
{
    const char* path = "gem_ped_3320.dat";
    fstream f(path, fstream::in);
    vector<TH1F*> res;

    vector<float> apv_offset;
    vector<float> apv_noise;
 
    string line;
    while(getline(f, line)) 
    {
        int mpd, adc, crate;
        istringstream iss(line);
        if(line.find("APV") != string::npos) {
            string tmp;
            iss >> tmp >> crate >> mpd >> adc;

            if(apv_offset.size() > 0) {
                res.push_back(plot_apv(apv_offset, crate, mpd, adc, "offset"));
                res.push_back(plot_apv(apv_noise, crate, mpd, adc, "noise"));
            }
            apv_offset.clear();
            apv_noise.clear();
        } else {
            int strip;
            float offset, noise;
            iss >> strip >> offset >> noise;
            apv_offset.push_back(offset);
            apv_noise.push_back(noise);
        }
    }

    // save histos
    TFile *froot = new TFile("pedestal.root", "recreate");
    for(auto &i: res)
        i->Write();
    froot->Close();

    draw();
}

