void plot_apv_time_bin()
{
    string file = "../gui/apv_time_sample_check.root";

    const int nC = 22;

    TCanvas *c[nC];
    for(int i=0;i<nC;i++) {
        c[i] = new TCanvas(Form("c%d", i), Form("c%d", i), 1000, 800);
        c[i]->Divide(5, 4);
    }
    gStyle->SetTitleW(0.7);
    gStyle->SetTitleH(0.08);

    TFile *f = new TFile(file.c_str());
    TIter keyList(f->GetListOfKeys());
    TKey *key;
    int i=0;
    while( (key=(TKey*)keyList()) )
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TGraph")) continue;

        TGraphErrors *g = (TGraphErrors*)key->ReadObj();
        g->GetYaxis()->SetLabelSize(0.06);
        g->GetXaxis()->SetLabelSize(0.06);


        int idx = i/20;
        int rem = i%20;

        c[idx]->cd(rem+1);
        g->Draw();
        i++;
    }

    for(int i=0;i<nC;i++)
        c[i] -> Print(Form("tmp_plots/c%d.eps", i), "eps");

    c[0] -> Print("tmp_plots/c.pdf(", "pdf");
    for(int i=1;i<nC-1;i++)
        c[i] -> Print("tmp_plots/c.pdf", "pdf");
    c[8] -> Print("tmp_plots/c.pdf)", "pdf");
}
