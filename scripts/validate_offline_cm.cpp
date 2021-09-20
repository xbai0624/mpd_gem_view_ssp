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

vector<string> load_txt(const char* path)
{
    vector<string> res;
    fstream f(path, iostream::in);
    if(!f.is_open()) cout<<"Error : cannot open file: "<<path<<endl;

    string line;
    while(getline(f,line)) {
        if(line.size() > 0)
            res.push_back(line);
        line.clear();
    }

    return res;
}

void validate_offline_cm()
{
    //vector<string> lines = load_txt("../log_offset_added_back_danning_5sigma.txt");
    vector<string> lines = load_txt("../log_new_firmware.txt");
    int n = (int)lines.size();
    if(n % 131 != 0) 
        cout<<"erro"<<endl;
    else
        cout<<"total time samples: "<<n/131<<endl;

    vector<vector<string>> frames;
    vector<string> tmp;
    for(int i=0; i<n; i++) {
        if( i%131 == 0 && i!= 0) {
            frames.push_back(tmp);
            tmp.clear();
        }
        tmp.push_back(lines[i]);
    }
    if(tmp.size() > 0) frames.push_back(tmp);

    cout<<"total frames: "<<frames.size()<<endl;

    // function to parse apv address
    auto parse_address = [&](const string &s)
        -> APVAddress
    {
        istringstream iss(s);
        string tmp;
        vector<string> cache;
        while(iss>>tmp) {
            cache.push_back(tmp);
            tmp.clear();
        }

        int crate_id = stoi(cache[2]);
        int mpd_id = stoi(cache[3]);
        int adc_ch = stoi(cache[4]);

        return APVAddress(crate_id, mpd_id, adc_ch);
    };

    // function to parse time sample number
    auto parse_ts = [&](const string &s)
        -> int
    {
        istringstream iss(s);
        string tmp;
        vector<string> cache;
        while(iss >> tmp) {
            cache.push_back(tmp);
            tmp.clear();
        }
        return stoi(cache[2]);
    };

    // function to parse offline cm
    auto parse_cm = [&](const string &s)
        -> vector<int>
    {
        istringstream iss(s);
        string tmp;
        vector<string> cache;
        while(iss >> tmp) {
            cache.push_back(tmp);
            tmp.clear();
        }

        string count_A_s = cache[4];
        string count_B_s = cache[9];
        string cm_s = cache[13];
        string cm_online_s = cache[17];

        int count_a = stoi(count_A_s.substr(0, count_A_s.size() - 1));
        int count_b = stoi(count_B_s.substr(0, count_B_s.size() - 1));
        int cm = stoi(cm_s.substr(0, cm_s.size() - 1));
        int cm_online = stoi(cm_online_s);

        return vector<int>({count_a, count_b, cm, cm_online});
    };

    // function to histogram frame
    auto show_frame = [&](const vector<string> &frame)
        -> vector<TH1F*>
    {
        APVAddress addr = parse_address(frame[0]);
        int time_sample = parse_ts(frame[1]);
        vector<int> cm_info = parse_cm(frame[130]);
        cout<<"processing apv: "<<addr.crate_id<<", "<<addr.mpd_id<<", "<<addr.adc_ch<<endl;
        cout<<"           ts: "<<time_sample<<endl;
        cout<<"           count_a: "<<cm_info[0]<<", count_b: "<<cm_info[1]<<", cm: "<<cm_info[2]<<endl;

        TH1F *h = new TH1F(Form("h_%d_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch, time_sample),
                Form("crate%d_mpd%d_adc%d_ts%d_countA%d_countB%d_cm%d", addr.crate_id, addr.mpd_id, addr.adc_ch, time_sample, cm_info[0], cm_info[1], cm_info[2]), 
                140, -5, 135);
        // a helper
        TH1F *cm = new TH1F(Form("hcm_%d_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch, time_sample), "cm", 140, -5, 135);
        TH1F *cm_online = new TH1F(Form("hcm_online_%d_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch, time_sample), "cm", 140, -5, 135);

        for(int i=2; i<130; i++) {
            int adc = stoi(frame[i]);
            h->SetBinContent(i+10, adc);
            cm -> SetBinContent(i+10, cm_info[2]);
            cm_online -> SetBinContent(i+10, cm_info[3]);
        }

        return vector<TH1F*>({h, cm, cm_online});
    };

    // to plots
    int N = frames.size();
    int index = 0;
    TCanvas *c[N/6];
    for(int i=0; i<N/6;i++) {
        c[i] = new TCanvas(Form("c%d", i), "c", 1200, 1000);
        c[i] -> Divide(3, 2);
    }
    for(auto &i: frames) {
        int major = index/6;
        int minor = index%6;
        c[major] -> cd(minor+1);
        vector<TH1F*> h = show_frame(i);
        h[0]->Draw();
        h[1] -> SetLineColor(2);
        h[1] -> Draw("same");
        h[2] -> SetLineColor(6);
        h[2] -> Draw("same");
        TLegend *leg = new TLegend(0.25, 0.2, 0.8, 0.4);
        leg -> AddEntry(h[0], "raw data", "lp");
        leg -> AddEntry(h[1], "offline common mode", "lp");
        leg -> AddEntry(h[2], "online common mode", "lp");
        leg -> Draw(); 
        index++;

        if(index/6 == (N/6 - 1)) break;
    }

    c[0] -> Print("common_mode.pdf(");
    for(int i=1; i<N/6-1; i++)
        c[i] -> Print("common_mode.pdf");
    c[N/6-1] -> Print("common_mode.pdf)");

}
