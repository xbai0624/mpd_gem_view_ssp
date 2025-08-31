void __format_histo(TH1F* g)
{
    double label_size = 0.045;                                                       
    double title_size = 0.055;
    //g->GetXaxis()->SetTitle(x_title.c_str());                                        
    g->GetXaxis()->SetLabelSize(label_size);                                         
    g->GetXaxis()->SetTitleSize(title_size);                                         
    g->GetXaxis()->SetLabelFont(62);                                                 
    g->GetXaxis()->SetTitleFont(62);
    g->GetXaxis()->SetTitleOffset(1.0);                                              
    g->GetXaxis()->CenterTitle();                                                    

    //g->GetYaxis()->SetTitle(y_title.c_str());                                        
    g->GetYaxis()->SetLabelSize(label_size);                                         
    g->GetYaxis()->SetTitleSize(title_size);                                         
    g->GetYaxis()->SetLabelFont(62);                                                 
    g->GetYaxis()->SetTitleFont(62);
    g->GetYaxis()->SetTitleOffset(1.1);                                              
    g->GetYaxis()->SetNdivisions(505);                                               
    g->GetYaxis()->CenterTitle();   
}


struct APVAddress
{
    int crate_id, mpd_id, adc_ch;

    APVAddress():crate_id(-1), mpd_id(-1), adc_ch(-1)
    {}

    APVAddress(int c, int m, int a):crate_id(c), mpd_id(m), adc_ch(a)
    {}

    APVAddress(const APVAddress &addr):crate_id(addr.crate_id),
    mpd_id(addr.mpd_id), adc_ch(addr.adc_ch)
    {}

    APVAddress & operator=(const APVAddress &addr) {
        crate_id = addr.crate_id, mpd_id = addr.mpd_id, adc_ch = addr.adc_ch;
        return *this;
    };

    bool operator==(const APVAddress &addr) const
    {
        return (crate_id == addr.crate_id) && (mpd_id == addr.mpd_id) && (adc_ch == addr.adc_ch);
    }
};

namespace std {
    template<> struct hash<APVAddress>
    {
        std::size_t operator()(const APVAddress &k) const
        {
            return ( (k.adc_ch & 0xf)
                    | ((k.mpd_id & 0x7f) << 4)
                    | ((k.crate_id & 0xff)<<11)
                   );
        }
    };
}

APVAddress parse_address(const string &line)
{
    if(line.find("apv") == string::npos)
    {
        cout<<"line does not contain apv address"<<endl;
    }

    istringstream iss(line);
    string tmp;
    vector<string> vec;
    while(iss >> tmp)
        vec.push_back(tmp);

    APVAddress addr;
    addr.crate_id = stoi(vec[1]);
    addr.mpd_id = stoi(vec[2]);
    addr.adc_ch = stoi(vec[3]);

    return addr;
}

void load_file(const char* path, unordered_map<APVAddress, vector<int>> &cache)
{
    //APVAddress apv_chosen(20, 8, 14); // only see this apv

    fstream f(path, fstream::in);
    string line;
    vector<int> apv;
    APVAddress addr;
    while(getline(f, line))
    {
        if(line.find("apv") != string::npos) {
            if(apv.size() > 0) {
                //if(addr == apv_chosen)
                    cache[addr] = apv;
                apv.clear();
            }
            addr = parse_address(line);
        }
        else {
            apv.push_back(stod(line));
        }
        line.clear();
    }
}

void plot_event_in_txt(const char* path = "../Rootfiles/event_3.txt")
{
    unordered_map<APVAddress, vector<int>> cache;
    load_file(path, cache);

    size_t n_apv = cache.size();
    cout<<"total apvs: "<<n_apv<<endl;

    vector<TH1F*> histos;
    for(auto &i: cache) {
        TH1F *h = new TH1F(Form("crate_%d_mpd_%d_adc_%d", i.first.crate_id, i.first.mpd_id, i.first.adc_ch), 
                Form("crate_%d_mpd_%d_adc_%d", i.first.crate_id, i.first.mpd_id, i.first.adc_ch), 780, 0, 780);
        int index = 1;
        for(auto &j: i.second)
        {
            h -> SetBinContent(index, j);
            index++;
        }

        h->GetXaxis() -> SetTitle("Time Sample");
        h->GetYaxis() -> SetTitle("ADC");
        __format_histo(h);
        histos.push_back(h);
    }

    int n_canvas = n_apv / 16 + 1;
    vector<TCanvas*> canvas;
    for(int i=0; i<n_canvas; i++) {
        TCanvas *c = new TCanvas(Form("c_%d", i), Form("c_%d", i), 1200, 900);
        c -> Divide(4, 4);
        canvas.push_back(c);
    }

    gStyle -> SetOptStat(0);

    // draw histos
    for(int i=0; i<n_apv; i++) {
        int nc = i / 16;
        int pos = i % 16;

        canvas[nc] -> cd(pos + 1);
        gPad -> SetFrameLineWidth(2);
        histos[i] -> Draw();
    }

    for(int i=0; i<n_canvas; i++)
        canvas[i] -> Draw();
}
