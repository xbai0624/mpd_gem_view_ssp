{
    const int NCanvas = 6;
    TCanvas *c[NCanvas];
    for(int i=0;i<NCanvas;i++) {
        c[i] = new TCanvas(Form("c_%d", i), Form("c_%d", i), 1600, 900);
        c[i]->Divide(6, 5);
    }

    TFile *f = new TFile("pedestal_2252.root");
    TIter keyList(f->GetListOfKeys());
    TKey *key;
    int i= 0;
    while((key = (TKey*)keyList()))
    {
        TClass *cl = gROOT -> GetClass(key->GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;
        TH1I* h = (TH1I*)key->ReadObj();

        string name = h->GetName();
        if(name.find("noise") == string::npos) continue;

        h->SetLineWidth(2);
        h->GetXaxis()->SetLabelSize(0.065);
        h->GetYaxis()->SetLabelSize(0.065);

        int n = i / 30;
        int j = i % 30;
        c[n]->cd(j+1);
        gPad->SetFrameLineWidth(2);

        gStyle->SetTitleFontSize(0.08);
        h->Draw();
        i++;
    }

    TCanvas *c1 = new TCanvas("c1", "c", 1000, 800);
    gPad->SetLogx();
    gPad->SetFrameLineWidth(2);
    gStyle->SetOptStat(0);
    TH1I* h1 = (TH1I*)f->Get("hOverallNoise");
    h1->SetTitle("Combined Overall RMS Noise");
    h1->SetLineWidth(2);
    h1->GetXaxis()->SetTitle("ADC");
    h1->GetXaxis()->CenterTitle();
    h1->GetYaxis()->SetTitle("Counts");
    h1->GetYaxis()->CenterTitle();
    h1->Draw();
    TFile *f2 = new TFile("pedestal_2069.root");
    TH1I* h2 = (TH1I*)f2->Get("hOverallNoise");
    h2->SetLineColor(2);
    double entry1 = h1->GetEntries();
    double entry2 = h2->GetEntries();
    h2->Scale(entry1/entry2);
    //h2->Draw("hist same");

    // save
    for(int i=0;i<NCanvas;i++)
        c[i]->Print(Form("plots/c%d.png", i));
    c1->Print("plots/overall_.png");
}
