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


void compare_offline_online_cm()
{
    //TFile *f = new TFile("../Rootfiles/cluster_0_bbgem_532.root");
    TFile *f = new TFile("../Rootfiles/cluster_0_bbgem_new_firmware_592.root");
    TTree *T = (TTree*)f -> Get("GEMCluster");

    int N = T->GetEntries();
    cout<<"total entries: "<<N<<endl;

    const int MAX = 500;
    int nAPV;
    int apv_crate_id[MAX];
    int apv_mpd_id[MAX];
    int apv_adc_ch[MAX];
    int cm0_offline[MAX];
    int cm1_offline[MAX];
    int cm2_offline[MAX];
    int cm3_offline[MAX];
    int cm4_offline[MAX];
    int cm5_offline[MAX];
    int cm0_online[MAX];
    int cm1_online[MAX];
    int cm2_online[MAX];
    int cm3_online[MAX];
    int cm4_online[MAX];
    int cm5_online[MAX];

    T -> SetBranchAddress("nAPV", &nAPV);
    T -> SetBranchAddress("apv_crate_id", apv_crate_id);
    T -> SetBranchAddress("apv_mpd_id", apv_mpd_id);
    T -> SetBranchAddress("apv_adc_ch", apv_adc_ch);
    T -> SetBranchAddress("CM0_offline", cm0_offline);
    T -> SetBranchAddress("CM1_offline", cm1_offline);
    T -> SetBranchAddress("CM2_offline", cm2_offline);
    T -> SetBranchAddress("CM3_offline", cm3_offline);
    T -> SetBranchAddress("CM4_offline", cm4_offline);
    T -> SetBranchAddress("CM5_offline", cm5_offline);
    T -> SetBranchAddress("CM0_online", cm0_online);
    T -> SetBranchAddress("CM1_online", cm1_online);
    T -> SetBranchAddress("CM2_online", cm2_online);
    T -> SetBranchAddress("CM3_online", cm3_online);
    T -> SetBranchAddress("CM4_online", cm4_online);
    T -> SetBranchAddress("CM5_online", cm5_online);

    unordered_map<APVAddress, TH1F*> offline_histos;
    unordered_map<APVAddress, TH1F*> online_histos;
    unordered_map<APVAddress, TH1F*> diff_histos;

    for(int entry = 0; entry < N; ++entry)
    {
        T -> GetEntry(entry);

        for(int i=0; i<nAPV;i++) {
            APVAddress addr(apv_crate_id[i], apv_mpd_id[i], apv_adc_ch[i]);

            if(offline_histos.find(addr) == offline_histos.end()) {
                offline_histos[addr] = new TH1F(Form("hoffline_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                        Form("offline_crate%d_mpd%d_adc%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 700, 100, 800);

                online_histos[addr] = new TH1F(Form("honline_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                        Form("online_crate%d_mpd%d_adc%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 700, 100, 800);

                diff_histos[addr] = new TH1F(Form("hdiff_%d_%d_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                        Form("diff_crate%d_mpd%d_adc%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 700, 100, 800);
            }

            offline_histos[addr] -> Fill(cm3_offline[i]);
            online_histos[addr] -> Fill(cm3_online[i]);
            diff_histos[addr] -> Fill(cm3_offline[i] - cm3_online[i]);
        }
    }

    int total_apvs = (int)offline_histos.size();
    int nc = total_apvs / 4 + 1;
    TCanvas *c[nc];
    for(int ic = 0; ic<nc; ++ic)
    {
        c[ic] = new TCanvas(Form("c%d", ic), "c", 800, 600);
        c[ic] -> Divide(2, 2);
    }
    int index = 0;
    for(auto &hist: offline_histos)
    {
        c[index/4] -> cd(index%4+1);
        if(hist.second -> GetEntries() <= 1000) continue;

        hist.second -> SetLineColor(2);
        THStack *hs = new THStack();
        hs->Add(hist.second);
        hs->Add(online_histos[hist.first]);
        hs->Draw("no stack");

        TLegend *leg = new TLegend(0.1, 0.75, 0.5, 0.9);
        leg -> AddEntry(hist.second, hist.second -> GetTitle(), "lp");
        leg -> AddEntry(online_histos[hist.first], online_histos[hist.first] -> GetTitle(), "lp");
        leg -> Draw();
        //diff_histos[hist.first] -> Draw("same");
        index++;
    }

    c[0] -> Print("res.pdf(");
    for(int k = 1; k<nc-1;++k)
        c[k] -> Print("res.pdf");
    c[nc-1] -> Print("res.pdf)");
}
