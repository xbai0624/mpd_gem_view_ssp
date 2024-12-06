
#include "MollerROStripDesign.hpp"

struct strip_t
{
    int strip_no;
    double x_start, y_start, x_end, y_end;

    strip_t() : strip_no(0), x_start(0), y_start(0),
    x_end (0), y_end(0)
    {
    }

    strip_t(int i, double x0, double y0, double x1, double y1)
        : strip_no(i), x_start(x0), y_start(y0), x_end(x1), y_end(y1)
    {
    }
};

map<int, strip_t> top_strips;
map<int, strip_t> bot_strips;

void plot()
{
    load_strip_design_files();

    fstream f("Moller_GEM_RO_Top_Strip_Coord.txt", fstream::in);
    int strip_no;
    double x_start, y_start, x_end, y_end;
    while(f >> strip_no >> x_start >> y_start >> x_end >> y_end)
    {
        strip_t s(strip_no, x_start, y_start, x_end, y_end);
        top_strips[strip_no] = s;
    }

    fstream fp("Moller_GEM_RO_Bot_Strip_Coord.txt", fstream::in);
    while(fp >> strip_no >> x_start >> y_start >> x_end >> y_end)
    {
        strip_t s(strip_no, x_start, y_start, x_end, y_end);
        bot_strips[strip_no] = s;
    }


    TCanvas *c = new TCanvas("c", "c", 800, 700);

    TH2F *frame = new TH2F("frame", "strips", 100, 0, 25, 100, 0, 25);
    int nb = 0;
    for(auto &i1: bot_strips)
    {
        cout<<"drawing bottom strip : "<<nb <<" and its intersecting top strips"<<endl;
        cout<<"Press Enter to continue..."<<endl;
        c -> Clear();
        frame -> Draw();

        nb = i1.first;
        auto &s1 = i1.second;
        TLine *line1 = new TLine(s1.x_start, s1.y_start, s1.x_end, s1.y_end);
        line1->SetLineColor(kBlue);
        line1->SetLineWidth(2);

        int nb2 = 0;
        for(auto &i2: top_strips) {
            auto & s2 = i2.second;
            nb2 = i2.first;

            //bool intersect = has_intersect(s1.x_start, s1.y_start, s1.x_end, s1.y_end,
            //        s2.x_start, s2.y_start, s2.x_end, s2.y_end);
            bool intersect = has_intersect_bot_top(nb, nb2);

            if(!intersect) continue;

            TLine *line2 = new TLine(s2.x_start, s2.y_start, s2.x_end, s2.y_end);
            line2->SetLineColor(kRed);
            line2->SetLineWidth(2);
            line2->Draw();
        }

        line1->Draw();
        c -> Update();
        gSystem -> ProcessEvents();

        nb++;
        getchar();
    }
}
