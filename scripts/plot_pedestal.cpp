#include <vector>
#include <iostream> 

//helper function for conversion to physical strip layout for MOLLERGEM any other INFN XY SRS based strip mapping
int strip_conversion(int ch, const std::string& detector_type = "default") 
{
    int physical_strip_ch = -1; 

    if (detector_type == "default") {
	return ch; 
    }

    if (detector_type == "MOLLERGEM" || detector_type == "INFN_XY") {
	physical_strip_ch = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);
	return physical_strip_ch; 
    }

    return physical_strip_ch; 
}

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
void plot_pedestal(const char* path = "../database/gem_ped.dat", const std::string& detector_type = "default")
{
    fstream f(path, std::fstream::in);
    if (!f.is_open()) {
	std::cerr << "Error opening file: " << path << std::endl; 
	return;
    }

    vector<TH1F*> res;
    vector<float> apv_offset;
    vector<float> apv_noise;
    vector<float> apv_noise_phys(128); 

    int slot, mpd, adc, crate;
    string line;
    while(getline(f, line)) 
    {
        istringstream iss(line);
        if(line.find("APV") != string::npos) //if current scanned line does have "APV"
        {
            if(apv_offset.size() > 0) { //make sure all data has been accounted for before moving on to next set of APV data
                res.push_back(plot_apv(apv_offset, crate, mpd, adc, "offset"));
                res.push_back(plot_apv(apv_noise, crate, mpd, adc, "noise"));
		res.push_back(plot_apv(apv_noise_phys, crate, mpd, adc, "noise_phys")); 
            }
            apv_offset.clear();
            apv_noise.clear();
	    apv_noise_phys.assign(128, 0); 

            string tmp;
            iss >> tmp >> crate >> slot >> mpd >> adc;
        } else {
            int strip;
            float offset, noise;
            iss >> strip >> offset >> noise;
            apv_offset.push_back(offset);
            apv_noise.push_back(noise);
	    apv_noise_phys[(strip_conversion(strip, detector_type))] = noise;  
        }
    }
    // save last apv
    if(apv_offset.size() > 0) {
        res.push_back(plot_apv(apv_offset, crate, mpd, adc, "offset"));
        res.push_back(plot_apv(apv_noise, crate, mpd, adc, "noise"));
	res.push_back(plot_apv(apv_noise_phys, crate, mpd, adc, "noise_phys"));
    }

    // save histos
    TFile *froot = new TFile("tmp_plots/pedestal.root", "recreate");
    for (size_t i = 0; i < res.size(); ++i) {
    res[i]->Write();
    }
    froot->Close();

    // plot histos
    int nMPD = res.size() / 3 / 16 + 1;
    TCanvas *c_offset[nMPD];
    TCanvas *c_noise[nMPD];
    TCanvas *c_noise_phys[nMPD]; 
    for(int k = 0;k<nMPD;k++) {
        c_offset[k] = new TCanvas(Form("c_offset_%d", k), Form("c_offset_%d", k), 1000, 800);
        c_offset[k] -> Divide(4, 4);
        c_noise[k] = new TCanvas(Form("c_noise_%d", k), Form("c_noise_%d", k), 1000, 800);
        c_noise[k] -> Divide(4, 4);
	c_noise_phys[k] = new TCanvas(Form("c_noise_phys_%d", k), Form("c_noise_phys_%d", k), 1000, 800); 
	c_noise_phys[k] -> Divide(4, 4); 
    }
    TFile *f_pedestal = new TFile("tmp_plots/pedestal.root");
    TIter keyList(f_pedestal->GetListOfKeys());
    TKey *key;
    int n_offset = 0, n_noise = 0, n_noise_phys = 0; 
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
	else if(title.find("noise") != string::npos && title.find("noise_phys") == std::string::npos)
	{
	    int nCanvas = n_noise / 16;
	    int nPad = n_noise % 16 + 1;
	    c_noise[nCanvas] -> cd(nPad);
	    h->Draw();

	    n_noise ++;
	}
	else if(title.find("noise_phys") != string::npos)
	{
	    int nCanvas = n_noise_phys / 16; 
	    int nPad = n_noise_phys % 16 + 1; 
	    c_noise_phys[nCanvas] -> cd(nPad); 
	    h->Draw(); 

	    n_noise_phys ++;
	}
    }
    
    c_offset[0]->Print("tmp_plots/pedestal.pdf("); // Open the PDF file with the first canvas
    
    for (int k = 0; k < nMPD; k++) {
      if (k > 0) c_offset[k]->Print("tmp_plots/pedestal.pdf");
      
      c_noise[k]->Print("tmp_plots/pedestal.pdf");
      
      if (k == nMPD - 1) {
        c_noise_phys[k]->Print("tmp_plots/pedestal.pdf)");
	
      } else {
        c_noise_phys[k]->Print("tmp_plots/pedestal.pdf");
	
      }
      
    }
    
}

