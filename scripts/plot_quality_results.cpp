

void plot_quality_results(const char* path = "../Rootfiles/cluster_0_fermilab_beamtest_8.evio.0.root_data_quality_check.root")
{
    TFile *f = new TFile(path);

    TIter keyList(f->GetListOfKeys());
    TKey *key;

    // get total number of histograms
    int nHistos = 0;
    while( (key = (TKey*)keyList()) )
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;
        nHistos++;
    }

    // draw histograms, 4 hists per canvas
    vector<TCanvas*> vCanvas;
    int counter = 0;
    keyList.Reset();
    while( (key = (TKey*)keyList()) )
    {
        TClass *cl = gROOT -> GetClass(key -> GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;

        TH1F* h = (TH1F*)key -> ReadObj();

        if(counter%4 == 0) {
            vCanvas.push_back( new TCanvas(Form("c%d",(int)vCanvas.size()), Form("c%d", (int)vCanvas.size()), 1200, 1000) );
            vCanvas.back() -> Divide(2, 2);
        }

        int npad = counter % 4 + 1;
        vCanvas.back() -> cd(npad);
        h -> Draw("colz");

        counter++;
    }
    cout<<"total histograms: "<<counter<<endl;

    // save to pdf
    string ofile;
    if(vCanvas.size() <= 0) {
        cout<<"Root file is empty. Nothing is plotted."<<endl;
        return;
    }
    if(vCanvas.size() <= 1) {
        ofile = string(path) + string(".pdf");
        vCanvas.back() -> Print(ofile.c_str());
        return;
    }

    ofile = string(path) + string(".pdf(");
    vCanvas[0] -> Print(ofile.c_str());

    for(int i=1; i<vCanvas.size() - 1; i++)
    {
        ofile = string(path) + string(".pdf");
        vCanvas[i] -> Print(ofile.c_str());
    }

    ofile = string(path) + string(".pdf)");
    vCanvas.back() -> Print(ofile.c_str());
}
