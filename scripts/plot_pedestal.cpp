
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

// plot pedestal
void plot_pedestal(const char* path = "../database/gem_ped.dat")
{
    fstream f(path, fstream::in);
    vector<TH1F*> res;

    vector<float> apv_offset;
    vector<float> apv_noise;

    int slot, mpd, adc, crate;
    string line;
    while(getline(f, line)) 
    {
        istringstream iss(line);
        if(line.find("APV") != string::npos)
        {
            if(apv_offset.size() > 0) {
                res.push_back(plot_apv(apv_offset, crate, mpd, adc, "offset"));
                res.push_back(plot_apv(apv_noise, crate, mpd, adc, "noise"));
            }
            apv_offset.clear();
            apv_noise.clear();

            string tmp;
            iss >> tmp >> crate >> slot >> mpd >> adc;
        } else {
            int strip;
            float offset, noise;
            iss >> strip >> offset >> noise;
            apv_offset.push_back(offset);
            apv_noise.push_back(noise);
        }
    }
    // save last apv
    if(apv_offset.size() > 0) {
        res.push_back(plot_apv(apv_offset, crate, mpd, adc, "offset"));
        res.push_back(plot_apv(apv_noise, crate, mpd, adc, "noise"));
    }

    // save histos
    TFile *froot = new TFile("tmp_plots/pedestal.root", "recreate");
    for(auto &i: res)
        i->Write();
    froot->Close();

    // plot histos
    int nMPD = res.size() / 2 / 16 + 1;
    TCanvas *c_offset[nMPD];
    TCanvas *c_noise[nMPD];
    for(int k = 0;k<nMPD;k++) {
        c_offset[k] = new TCanvas(Form("c_offset_%d", k), Form("c_offset_%d", k), 1000, 800);
        c_offset[k] -> Divide(4, 4);
        c_noise[k] = new TCanvas(Form("c_noise_%d", k), Form("c_noise_%d", k), 1000, 800);
        c_noise[k] -> Divide(4, 4);
    }
    TFile *f_pedestal = new TFile("tmp_plots/pedestal.root");
    TIter keyList(f_pedestal->GetListOfKeys());
    TKey *key;
    int n_offset = 0, n_noise = 0;
    while( (key = (TKey*)keyList()) ){
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;

        TH1F* h = (TH1F*)key->ReadObj();
        if(h->GetBinContent(10) == 5000 || h->GetBinContent(10) == 0) continue;

        string title = h->GetTitle();
	if(title.find("offset") != string::npos)
	{
	    int nCanvas = n_offset / 16;
	    int nPad = n_offset % 16 + 1;
	    c_offset[nCanvas] -> cd(nPad);
	    h->Draw();

	    n_offset ++;
	}
	else if(title.find("noise") != string::npos)
	{
	    int nCanvas = n_noise / 16;
	    int nPad = n_noise % 16 + 1;
	    c_noise[nCanvas] -> cd(nPad);
	    h->Draw();

	    n_noise ++;
	}
    }
}

