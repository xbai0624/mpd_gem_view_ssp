void show_all_channels()
{
    const int NCH = 16;

    TFile *f[NCH];
    TMultiGraph *mg[NCH];

    for(int i=0;i<NCH;i++)
    {
        f[i] = new TFile(Form("res_channel%d.root", i));
        mg[i] = (TMultiGraph*)f[i] -> Get(Form("mg_ch%d", i));
    }

    TCanvas *c = new TCanvas("c", "c", 800, 900);
    c->Divide(4, NCH/4);

    for(int i=0;i<NCH;i++)
    {
        c->cd(i+1);
        mg[i] -> SetTitle(Form("channel %d", i));
        mg[i]->Draw("apl");
    }

}
