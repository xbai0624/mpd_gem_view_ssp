{
    double x[] = {
        TMath::Power(10, 4), 
        TMath::Power(20, 4), 
        TMath::Power(30, 4), 
        TMath::Power(40, 4), 
        TMath::Power(50, 4), 
        TMath::Power(60, 4), 
        //TMath::Power(70, 4), 
        //TMath::Power(80, 4), 
        //TMath::Power(90, 4), 

    };

    double t[] = {
        5,
        57.5,
        263,
        783.,
        1870,
        3645,
    };

    TGraph *g = new TGraph(6, x, t);
    g->Fit("pol1");
    g -> Draw("apl*");
}
