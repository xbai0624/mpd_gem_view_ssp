

void plot_quality_results(const char* path = "../Rootfiles/cluster_0_fermilab_beamtest_290..root_data_quality_check.root")
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
    int total_triggers = 0;
    map<int, int> m_layer_x_nhits, m_layer_y_nhits;
    int total_tracks = 0;
    map<int, int> m_layer_didhits;
    while( (key = (TKey*)keyList()) )
    {
        TClass *cl = gROOT -> GetClass(key -> GetClassName());
        if(!cl->InheritsFrom("TH1")) continue;

        TH1F* h = (TH1F*)key -> ReadObj();

        if(counter % 4 == 0) {
            vCanvas.push_back( new TCanvas(Form("c%d",(int)vCanvas.size()), Form("c%d", (int)vCanvas.size()), 1200, 1000) );
            vCanvas.back() -> Divide(2, 2);
        }

        int npad = counter % 4 + 1;
        vCanvas.back() -> cd(npad);
        h -> Draw("colz");

        // calculate multiplicity based gem efficiency
	string h_name = h->GetName();
	if(h_name == "h_event_number") {
	    total_triggers = h -> GetEntries();
	}
	else if(h_name.find("cluster_multiplicity") != string::npos)
	{
	    // h_raw_x_cluster_multiplicity_layer${NGEM}
	    if(h_name.find("x") != string::npos) {
		char s_layer = h_name.back();
		int layer = s_layer - '0';

		m_layer_x_nhits[layer] = h -> GetEntries();
	    }
	    else if(h_name.find("y") != string::npos) {
		char s_layer = h_name.back();
		int layer = s_layer - '0';

		m_layer_y_nhits[layer] = h -> GetEntries();
	    }
	}

	// calculate tracking based gem efficiency
	else if(h_name.find("h_didhit_xy")!=string::npos)
	{
	    // h_didhit_xy_gem0
	    char s_layer = h_name.back();
	    int layer = s_layer - '0';
	    m_layer_didhits[layer] = h -> GetEntries();
	}
	else if(h_name.find("h_shouldhit_xy")!=string::npos)
	{
	    total_tracks = h -> GetEntries();
	}

        counter++;
    }
    cout<<"total histograms: "<<counter<<endl;

    // calculate multiplicity based efficiency
    TCanvas *c_eff = new TCanvas("c_efficiency", "efficiency by layer", 1200, 1000);
    TLatex latex;
    latex.SetTextSize(0.025);
    int _c = 1;
    latex.DrawLatex(0.05, 0.9, "cluster-multiplicity-based detector efficiency:");
    for(auto &i: m_layer_x_nhits) {
	string x_str_eff = Form("x axis effciency:(counts=%d/triggers=%d): layer = %d, eff = %.2f",
	    i.second, total_triggers, i.first, (float)i.second / (float)total_triggers );
	latex.DrawLatex(0.1, 0.85 - .05*_c, x_str_eff.c_str());
	_c++;
    }
    for(auto &i: m_layer_y_nhits) {
	string y_str_eff = Form("y axis effciency:(counts=%d/triggers=%d): layer = %d, eff = %.2f",
	    i.second, total_triggers, i.first, (float)i.second / (float)total_triggers );
	latex.DrawLatex(0.1, 0.75 - .05*_c, y_str_eff.c_str());
	_c++;
    }

    // calculate tracking based efficiency
    TCanvas *c_tracking_eff = new TCanvas("c_tracking_eff", "efficiency by tracking", 1200, 1000);
    _c = 1;
    latex.DrawLatex(0.05, 0.9, "tracking-based detector efficiency:");
    for(auto &i: m_layer_didhits)
    {
	string tracking_eff = Form("Layer %d Efficiency (detected_2d_hits=%d/total_tracks=%d) : %.2f", i.first, i.second, total_tracks, (float)i.second/(float)total_tracks);
	latex.DrawLatex(0.1, 0.85 - .05*_c, tracking_eff.c_str());
	_c++;
    }
    string tracking_eff = Form("Overall system tracking efficiency (total_tracks=%d/total_triggers=%d): %.2f", total_tracks, total_triggers, (float)total_tracks/(float)total_triggers);
    latex.DrawLatex(0.1, 0.65-0.05*_c, tracking_eff.c_str());

    // save to pdf
    string ofile;
    if(vCanvas.size() <= 0) {
        cout<<"Root file is empty. Nothing is plotted."<<endl;
        return;
    }

    ofile = string(path) + string(".pdf(");
    // multiplicity based efficiency
    c_eff -> Print(ofile.c_str());

    if(vCanvas.size() <= 1) {
        ofile = string(path) + string(".pdf");
        vCanvas.back() -> Print(ofile.c_str());
        return;
    }
    // tracking based efficiency
    c_tracking_eff -> Print(ofile.c_str());

    vCanvas[0] -> Print(ofile.c_str());

    for(int i=1; i<vCanvas.size() - 1; i++)
    {
        ofile = string(path) + string(".pdf");
        vCanvas[i] -> Print(ofile.c_str());
    }

    ofile = string(path) + string(".pdf)");
    vCanvas.back() -> Print(ofile.c_str());

}
