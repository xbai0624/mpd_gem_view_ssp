#include "HistoWidget.h"
#include <iostream>
#include <QHBoxLayout>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
// ctor

HistoWidget::HistoWidget(QWidget *parent): QWidget(parent)
{
    scene = new QGraphicsScene(this);
    //view = new HistoView(scene);
    view = new QGraphicsView(this);
    view -> setScene(scene);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(view);

    ReInitHistoItems();
    ReDistributePaintingArea();

    // disable touch events, otherwise it pops out a harmless warning
    view -> viewport() -> setAttribute(Qt::WA_AcceptTouchEvents, false);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

HistoWidget::~HistoWidget()
{
}

////////////////////////////////////////////////////////////////////////////////
// resize

void HistoWidget::resize([[maybe_unused]]int x, [[maybe_unused]]int y)
{
}

////////////////////////////////////////////////////////////////////////////////
// resize event

void HistoWidget::resizeEvent([[maybe_unused]]QResizeEvent *e)
{
    ReDistributePaintingArea();
}

////////////////////////////////////////////////////////////////////////////////
// mouse press event

void HistoWidget::mousePressEvent([[maybe_unused]]QMouseEvent *e)
{
    // place holder
}

////////////////////////////////////////////////////////////////////////////////
// init histoitems

void HistoWidget::ReInitHistoItems()
{
    pItem.clear();
    pItem.resize(fCol*fRow);

    for(int i=0;i<fCol*fRow;i++) {
        EnsureSlotType(i, PlotData::Plot1D);
    }
}

void HistoWidget::EnsureSlotType(int idx, PlotData::Type type)
{
    if(idx < 0 || idx >= static_cast<int>(pItem.size()))
        return;

    PlotSlot &slot = pItem[idx];
    if(slot.item && slot.type == type)
        return;

    if(slot.item) {
        scene->removeItem(slot.item);
        delete slot.item;
        slot.item = nullptr;
    }

    slot.type = type;
    slot.item = (type == PlotData::Plot2D)
        ? static_cast<QGraphicsItem*>(new HistoItem2D())
        : static_cast<QGraphicsItem*>(new HistoItem());
    scene->addItem(slot.item);
}

////////////////////////////////////////////////////////////////////////////////
// similar to root divide canvas

void HistoWidget::Divide(int r, int c)
{
    // clear all previous graphicsitem
    scene -> clear();

    // clear all previous histoitems
    // Correct:: no need to clear, QGraphicsScene takes ownership of items added 
    // to it through addItem(QGraphicsItem* item) function,
    // since you used "scene -> clear();", previous graphicsitem has already been
    // deleted
    //
    //for(int i=0;i<fCol*fRow;i++) {
    //    if(pItem[i])
    //        delete pItem[i];
    //}

    fCol = c; fRow = r;

    // re-init histoitems
    ReInitHistoItems();

    // re-distribute painting area
    ReDistributePaintingArea();
}

////////////////////////////////////////////////////////////////////////////////
// distribute painting area

void HistoWidget::ReDistributePaintingArea()
{
    int w = width();
    int h = height();

    // this margin is for hosting scroll bar etc
    float margin_x = 20;
    float margin_y = 50;
    w = w - margin_x;
    h = h - margin_y;

    // fix scene rect
    scene -> setSceneRect(0, 0, w, h);

    float x_interval = w / fCol;
    float y_interval = h / fRow;

    for(int i=0;i<fRow;i++) {
        for(int j=0;j<fCol;j++)
        {
            QRectF f(x_interval*j, y_interval*i, x_interval, y_interval);
            PlotSlot &slot = pItem[i*fCol + j];
            if(slot.type == PlotData::Plot2D)
                static_cast<HistoItem2D*>(slot.item)->SetBoundingRect(f);
            else
                static_cast<HistoItem*>(slot.item)->SetBoundingRect(f);
        }
    }

    Refresh();
}

////////////////////////////////////////////////////////////////////////////////
// distribute painting area

void HistoWidget::Refresh()
{
    for(int i=0;i<fRow*fCol;i++)
        if(pItem[i].item)
            pItem[i].item -> update();

    update();
}

////////////////////////////////////////////////////////////////////////////////
// distribute painting area

void HistoWidget::DrawCanvas(const std::vector<std::vector<int>> &data, 
        const std::vector<APVAddress> &addr, int row, int col)
{
    if(row != fRow || col != fCol)
    {
        Divide(row, col);
    }

    assert(static_cast<int>(data.size()) <= fRow * fCol);

    // fill histoitems
    for(unsigned int i=0; i < data.size(); i++) {
        EnsureSlotType(i, PlotData::Plot1D);
        HistoItem *item = static_cast<HistoItem*>(pItem[i].item);
        item -> ReceiveContents(data[i]);
        std::string title = "slot_" + std::to_string(addr[i].crate_id) +
            "_fiber_" + std::to_string(addr[i].mpd_id) + 
            "_apv_" + std::to_string(addr[i].adc_ch);
        item -> SetTitle(title);
    }
}

////////////////////////////////////////////////////////////////////////////////
// distribute painting area, get title from parameters

void HistoWidget::DrawCanvas(const std::vector<std::vector<int>> &data, 
        const std::vector<std::string> &vTitle, int row, int col)
{
    if(row != fRow || col != fCol)
    {
        Divide(row, col);
    }

    assert(static_cast<int>(data.size()) <= fRow * fCol);

    // fill histoitems
    for(unsigned int i=0; i < data.size(); i++) {
        EnsureSlotType(i, PlotData::Plot1D);
        HistoItem *item = static_cast<HistoItem*>(pItem[i].item);
        item -> ReceiveContents(data[i]);
        item -> SetTitle(vTitle[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////
// draw mixed 1D/2D histograms

void HistoWidget::DrawCanvas(const std::vector<PlotData> &data, int row, int col)
{
    if(row != fRow || col != fCol)
        Divide(row, col);

    assert(static_cast<int>(data.size()) <= fRow * fCol);

    for(int i=0;i<fRow*fCol;i++) {
        if(i >= static_cast<int>(data.size())) {
            EnsureSlotType(i, PlotData::Plot1D);
            static_cast<HistoItem*>(pItem[i].item)->clearContent();
            continue;
        }

        const PlotData &plot = data[i];
        EnsureSlotType(i, plot.type);
        if(plot.type == PlotData::Plot2D) {
            HistoItem2D *item = static_cast<HistoItem2D*>(pItem[i].item);
            item->SetTitle(plot.title);
            item->SetStats(plot.stats);
            item->SetData(plot.nx, plot.xMin, plot.xMax,
                          plot.ny, plot.yMin, plot.yMax, plot.z);
        } else {
            HistoItem *item = static_cast<HistoItem*>(pItem[i].item);
            item->SetTitle(plot.title);
            item->SetStats(plot.stats);
            item->ReceiveContents(plot.y);
        }
    }

    ReDistributePaintingArea();
}

////////////////////////////////////////////////////////////////////////////////
// clear

void HistoWidget::Clear()
{
    for(int i=0;i<fRow*fCol;i++) {
        EnsureSlotType(i, PlotData::Plot1D);
        static_cast<HistoItem*>(pItem[i].item)->clearContent();
    }
}
