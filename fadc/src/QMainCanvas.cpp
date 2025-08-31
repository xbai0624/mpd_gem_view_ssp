#include "QMainCanvas.h"

#include <TH1F.h>
#include <TSystem.h>
#include <TStyle.h>

#include <iostream>

#include <QMouseEvent>

////////////////////////////////////////////////////////////////////////////////
// ctor

QMainCanvas::QMainCanvas(QWidget* parent) : QWidget(parent)
{
    fCanvas = new QRootCanvas(this);
    layout = new QVBoxLayout(this);
    layout->addWidget(fCanvas);

    fRootCanvas = fCanvas->GetCanvas();

    connect(fCanvas, SIGNAL(ItemSelected()), this, SIGNAL(ItemSelected()));
    connect(fCanvas, SIGNAL(ItemDeSelected()), this, SIGNAL(ItemDeSelected()));

    fRootTimer = new QTimer(this);
    QObject::connect(fRootTimer, SIGNAL(timeout()), this, SLOT(handle_root_events()));
    fRootTimer->start(20);
}

////////////////////////////////////////////////////////////////////////////////
// drawing interface

void QMainCanvas::DrawCanvas(const std::vector<TH1I*> &hists, int w, int h)
{
    gStyle->SetOptStat(0);
    fRootCanvas->Clear();
    fRootCanvas->Divide(w, h);

    int ii = 0;
    for(auto &i: hists) {
        fRootCanvas->cd(ii+1);
        i -> Draw();
        ii++;
    }

    fRootCanvas->Modified();
    fRootCanvas->Update();
}

////////////////////////////////////////////////////////////////////////////////
// drawing interface, overloaded

void QMainCanvas::DrawCanvas(const std::vector<std::vector<int>> &hists, 
        const std::vector<APVAddress> &addr, int w, int h)
{
    gStyle->SetOptStat(0);
    fRootCanvas->Clear();
    fRootCanvas->Divide(w, h);

    int ii = 0;
    GenerateHistos(hists, addr);
    for(auto &i: vHContents) {
        fRootCanvas->cd(ii+1);
        i -> Draw();
        ii++;
    }

    fRootCanvas->Modified();
    fRootCanvas->Update();
}


////////////////////////////////////////////////////////////////////////////////
// drawing interface, overloaded

void QMainCanvas::DrawCanvas(const std::vector<TMultiGraph*> &mg, int w, int h)
{
    fRootCanvas -> Clear();
    fRootCanvas -> Divide(w, h);
    int ii = 1;
    for(auto &i: mg) 
    {
        fRootCanvas -> cd(ii);
        i -> Draw("apl");
        ii++;
    }
    fRootCanvas->Modified();
    fRootCanvas->Update();
}
 
////////////////////////////////////////////////////////////////////////////////
// root timer

void QMainCanvas::handle_root_events()
{
    gSystem->ProcessEvents();
}

////////////////////////////////////////////////////////////////////////////////
// slot mouse press event

void QMainCanvas::mousePressEvent(QMouseEvent *e)
{
    switch(e->button()) {
        case Qt::LeftButton:
            emit ItemSelected();
            break;
        case Qt::RightButton:
            emit ItemDeSelected();
            break;
        case Qt::MiddleButton:
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// get root canvas

TCanvas * QMainCanvas::GetCanvas()
{
    return fCanvas->GetCanvas();
}

////////////////////////////////////////////////////////////////////////////////
// refresh

void QMainCanvas::Refresh()
{
    fCanvas->Refresh();
}

////////////////////////////////////////////////////////////////////////////////
// slot paint event

void QMainCanvas::paintEvent([[maybe_unused]] QPaintEvent *e)
{
    Refresh();
}

////////////////////////////////////////////////////////////////////////////////
// slot resize event

void QMainCanvas::resizeEvent([[maybe_unused]] QResizeEvent *e)
{
    Refresh();
}


////////////////////////////////////////////////////////////////////////////////
// generate histos from vector

void QMainCanvas::GenerateHistos(const std::vector<std::vector<int>> &data, 
        const std::vector<APVAddress> &addr)
{
    // delete previous events
    for(auto &i: vHContents)
        if(i)
            delete i;
    vHContents.clear();

    // generate new pedestal
    auto get_histo = [&](const std::vector<int> &v, const APVAddress &addr) -> TH1I*
    {
        TH1I * h = new TH1I(Form("h_crate_%d_mpd_%d_slot_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                Form("h_crate_%d_mpd_%d_slot_%d", addr.crate_id, addr.mpd_id, addr.adc_ch), 
                v.size(), 0, v.size());
        for(unsigned int i=0;i<v.size();i++)
            h->SetBinContent(i, v[i]);
        return h;
    };

    for(unsigned int i=0;i<data.size();i++)
        vHContents.push_back(get_histo(data[i], addr[i]));
}
