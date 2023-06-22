

double get_sigma(TH1F *h, double low = 0, double high = 0)
{
    if(!h) return 0;
    int binmax = h -> GetMaximumBin();
    double x = h -> GetXaxis() -> GetBinCenter(binmax);
    double w = 0.25;
    cout<<"max position: "<<x<<endl;
    cout<<"fitting range: "<<x-w<<", "<<x+w<<endl;
    h -> Fit("gaus", "Q", "", x-w, x+w);
    double res = h -> GetFunction("gaus") -> GetParameter(2);

    return res;
}


vector<double> ana_file(int run_number)
{
    // ../Rootfiles/cluster_0_fermilab_beamtest_373..root_data_quality_check.root
    string fl1 = "../Rootfiles/cluster_0_fermilab_beamtest_" + to_string(run_number) + "..root_data_quality_check.root";
    cout<<"analyzing file: "<<fl1<<std::endl;
    TFile *f = new TFile(fl1.c_str());

    double ft1_x, ft1_y, ft2_x, ft2_y, tg4_x, tg4_y, rt1_x, rt1_y, rt2_x, rt2_y;

    // ft1_x
    TH1F *hft1_x = (TH1F*) f -> Get("h_xresid_gem0_exclusive");
    ft1_x = get_sigma(hft1_x);
    cout<<"ft1_x = "<<ft1_x<<endl;
    // ft1_y
    TH1F *hft1_y = (TH1F*) f -> Get("h_yresid_gem0_exclusive");
    ft1_y = get_sigma(hft1_y);
    cout<<"ft1_y = "<<ft1_y<<endl;
    // ft2_x
    TH1F *hft2_x = (TH1F*) f -> Get("h_xresid_gem1_exclusive");
    ft2_x = get_sigma(hft2_x);
    cout<<"ft2_x = "<<ft2_x<<endl;
    // ft2_y
    TH1F *hft2_y = (TH1F*) f -> Get("h_yresid_gem1_exclusive");
    ft2_y = get_sigma(hft2_y);
    cout<<"ft2_y = "<<ft2_y<<endl;
    /*
    // tg4_x
    TH1F *htg4_x = (TH1F*) f -> Get("h_xresid_gem4_exclusive");
    tg4_x = get_sigma(htg4_x);
    cout<<"tg4_x = "<<tg4_x<<endl;
    // tg4_y
    TH1F *htg4_y = (TH1F*) f -> Get("h_yresid_gem4_exclusive");
    tg4_y = get_sigma(htg4_y);
    cout<<"tg4_y = "<<tg4_y<<endl;
    */
    // rt1_x
    TH1F *hrt1_x = (TH1F*) f -> Get("h_xresid_gem5_exclusive");
    rt1_x = get_sigma(hrt1_x);
    cout<<"rt1_x = "<<rt1_x<<endl;
    // rt1_y
    TH1F *hrt1_y = (TH1F*) f -> Get("h_yresid_gem5_exclusive");
    rt1_y = get_sigma(hrt1_y);
    cout<<"rt1_y = "<<rt1_y<<endl;
    // rt2_x
    TH1F *hrt2_x = (TH1F*) f -> Get("h_xresid_gem6_exclusive");
    rt2_x = get_sigma(hrt2_x);
    cout<<"rt2_x = "<<rt2_x<<endl;
    // rt2_y
    TH1F *hrt2_y = (TH1F*) f -> Get("h_yresid_gem6_exclusive");
    rt2_y = get_sigma(hrt2_y);
    cout<<"rt2_y = "<<rt2_y<<endl;

    f -> Close();

    vector<double> res = { ft1_x, ft1_y, ft2_x, ft2_y, tg4_x, tg4_y, rt1_x, rt1_y, rt2_x, rt2_y};

    return res;
}

TGraph * plot(const vector<double> &xv, const vector<double> &yv)
{
    int n = xv.size();

    double *x = new double[n];
    double *y = new double[n];

    for(int i=0; i<n; i++) {
	x[i] = xv.at(i);
	y[i] = yv.at(i);
    }

    TGraph *g = new TGraph(n, x, y);
    return g;
}

void resolution()
{
    vector<double> res[6];
    for(int i=373; i<379; i++)
    {
	res[i-373] = ana_file(i);
    }

    vector<double> angle = {9, 15, 24, 30, 36, 42};

    vector<double> v_ft1_x, v_ft1_y, v_ft2_x, v_ft2_y, v_tg4_x, v_tg4_y, v_rt1_x, v_rt1_y, v_rt2_x, v_rt2_y;

    for(int i=0; i<6; i++)
    {
	v_ft1_x.push_back(res[i].at(0));
	v_ft1_y.push_back(res[i].at(1));
	v_ft2_x.push_back(res[i].at(2));
	v_ft2_y.push_back(res[i].at(3));
	v_tg4_x.push_back(res[i].at(4));
	v_tg4_y.push_back(res[i].at(5));
	v_rt1_x.push_back(res[i].at(6));
	v_rt1_y.push_back(res[i].at(7));
	v_rt2_x.push_back(res[i].at(8));
	v_rt2_y.push_back(res[i].at(9));
    }

    TGraph *g[10];
    g[0] = plot(angle, v_ft1_x);
    g[0] -> SetLineColor(1);

    g[1] = plot(angle, v_ft1_y);
    g[1] -> SetLineColor(2);

    g[2] = plot(angle, v_ft2_x);
    g[2] -> SetLineColor(3);

    g[3] = plot(angle, v_ft2_y);
    g[3] -> SetLineColor(4);

    g[4] = plot(angle, v_tg4_x);
    g[4] -> SetLineColor(5);

    g[5] = plot(angle, v_tg4_y);
    g[5] -> SetLineColor(6);

    g[6] = plot(angle, v_rt1_x);
    g[6] -> SetLineColor(7);

    g[7] = plot(angle, v_rt1_y);
    g[7] -> SetLineColor(8);

    g[8] = plot(angle, v_rt2_x);
    g[8] -> SetLineColor(9);

    g[9] = plot(angle, v_rt2_y);
    g[9] -> SetLineColor(10);

    TMultiGraph *mg = new TMultiGraph();
    for(int i=0; i<10; i++) {
	g[i] -> SetMarkerStyle(20);
	g[i] -> SetLineWidth(2);
	//mg -> Add(g[i]);
    }
    g[0] -> SetTitle("front tracker resolution");
    mg -> Add(g[0]);
    mg -> Add(g[1]);
    mg -> Add(g[2]);
    mg -> Add(g[3]);


    TCanvas *c = new TCanvas("c", "c", 900, 600);

    mg -> Draw("apl");

    mg -> GetXaxis()->SetTitle("angle [degree]");
    mg -> GetYaxis() -> SetTitle("resolution (mm)");
    mg -> SetTitle("front tracker resolution");

    c -> Modified();
    c -> Update();
}
