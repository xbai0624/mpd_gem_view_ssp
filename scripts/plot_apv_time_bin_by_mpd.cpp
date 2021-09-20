
struct MPDAddress
{
    int crate, mpd;

    MPDAddress(int c, int m)
        : crate(c), mpd(m)
    {}

    bool operator==(const MPDAddress &addr)
    {
        if(addr.crate == crate && addr.mpd == mpd )
            return true;
        return false;
    }
};

vector<int> findLocation(const string &str, char findIt)
{
    vector<int> res;
    for(int i=0;i<str.size();i++)
        if(str[i] == findIt)
            res.push_back(i);
    return res;
}

MPDAddress GetMPDAddress(const string &str)
{
    // g_crate_7_mpd_6_adc_13
    vector<int> pos = findLocation(str, '_');

    string _crate = str.substr(pos[1]+1, pos[2]-pos[1]-1);
    string _mpd = str.substr(pos[3]+1, pos[4]-pos[3]-1);

    int crate = TString(_crate).Atoi();
    int mpd = TString(_mpd).Atoi();

    MPDAddress addr(crate, mpd);

    return addr;
}

void plot_apv_time_bin_by_mpd()
{
    //string file = "../gem/apv_time_sample_check.root";
    string file = "../apv_time_sample_check.root";

    vector<TCanvas*> vCanvas;

    TFile *f = new TFile(file.c_str());
    TIter keyList(f->GetListOfKeys());
    TKey *key;
    int i = 0;
    MPDAddress prev_addr(-1, -1);

    while( (key = (TKey*)keyList()) )
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TGraph")) continue;

        TGraphErrors *g = (TGraphErrors*)key->ReadObj();

        string title = g->GetTitle();
        MPDAddress addr = GetMPDAddress(title);
        if( !(addr == prev_addr))
        {
            prev_addr = addr;
            i = 0;
            TCanvas *c = new TCanvas(Form("c%d", static_cast<int>(vCanvas.size())), "c", 1000, 800);
            c->Divide(4, 4);
            vCanvas.push_back(c);
        }

        int index = static_cast<int>(vCanvas.size()) -1 ;
        if(index > 100)  {
            cout<<"found more than 100 MPDs, something is wrong."<<endl;
            break;
        }
        vCanvas[index] -> cd(i+1);
        g->Draw("apl");
        i++;
    }

    for(auto &i: vCanvas)
        i->Draw();

    // save to file
    int s = static_cast<int>(vCanvas.size());
    vCanvas[0] -> Print("tmp_plots/average_time_bin_per_mpd.pdf[");
    for(int i=1;i<s-1;i++)
        vCanvas[i]-> Print("tmp_plots/average_time_bin_per_mpd.pdf");
    vCanvas[s-1]-> Print("tmp_plots/average_time_bin_per_mpd.pdf]");
}
