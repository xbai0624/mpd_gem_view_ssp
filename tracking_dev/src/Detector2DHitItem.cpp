#include "Detector2DHitItem.h"
#include "AbstractDetector.h"
#include <iostream>
#include <cmath>

namespace tracking_dev {

////////////////////////////////////////////////////////////////////////////////
// ctor

Detector2DHitItem::Detector2DHitItem()
{
    // default bounding rect
    _boundingRect.setRect(0, 0, 50, 50);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

Detector2DHitItem::~Detector2DHitItem()
{
}

////////////////////////////////////////////////////////////////////////////////
// bounding rect

QRectF Detector2DHitItem::boundingRect() const
{
    return _boundingRect;
}

////////////////////////////////////////////////////////////////////////////////
// paint

void Detector2DHitItem::paint(QPainter *painter,
        [[maybe_unused]] const QStyleOptionGraphicsItem *option,
        [[maybe_unused]] QWidget *widget)
{
    UpdateDrawingRange();

    // draw grids
    DrawGrids(painter);

    // draw a frame
    DrawAxis(painter);

    // draw event contents
    DrawEventContent(painter);
}

////////////////////////////////////////////////////////////////////////////////
// resize event

void Detector2DHitItem::resizeEvent()
{
    QPainter painter;
    paint(&painter);
}

////////////////////////////////////////////////////////////////////////////////
// set bounding rect

void Detector2DHitItem::SetBoundingRect(const QRectF &f)
{
    prepareGeometryChange(); // only needed when bounding rect changes

    _boundingRect = mapRectFromScene(f);
}

////////////////////////////////////////////////////////////////////////////////
// update range: update the drawing area range

void Detector2DHitItem::UpdateDrawingRange()
{
    QRectF default_range = boundingRect();

    // QGraphicsItem drawing area range
    // 10% pixel away from the bounding rect
    float ratio_away = 0.1;
    margin_x = ratio_away * default_range.width();
    margin_y = ratio_away * default_range.height();

    area_x1 = default_range.x() + margin_x;
    area_x2 = area_x1 + default_range.width() - 2. * margin_x;
    area_y1 = default_range.y() + margin_y;
    area_y2 = area_y1 + default_range.height() - 2. * margin_y;
}

////////////////////////////////////////////////////////////////////////////////
// draw a frame around the canvas area

void Detector2DHitItem::DrawAxis(QPainter *painter)
{
    QPen pen(Qt::black, 1);
    painter -> setPen(pen);
    // draw axis
    // (area_x1, area_y1, area_x2, area_y2)
    painter -> drawLine(area_x1, area_y1, area_x1, area_y2);
    painter -> drawLine(area_x1, area_y2, area_x2, area_y2);
    painter -> drawLine(area_x2, area_y2, area_x2, area_y1);
    painter -> drawLine(area_x1, area_y1, area_x2, area_y1);

    // draw title
    QFontMetrics fm = painter->fontMetrics();
    int text_width = fm.horizontalAdvance(_title), text_height = fm.height();
    int x_text = (area_x1 + area_x2) / 2 - text_width / 2, 
        y_text = area_y1 - text_height / 2;
    painter -> drawText(x_text, y_text, _title);
}

////////////////////////////////////////////////////////////////////////////////
// draw detector grids

void Detector2DHitItem::DrawGrids(QPainter *painter)
{
    QPen pen(Qt::lightGray, 1);
    //painter -> setPen(pen);

    const auto &grids = detector->GetGrids();
    const auto &grid_chosen = detector->GetGridChosen();

    for(auto &i: grids)
    {
        QPointF p1 = Coord(i.second.x1, i.second.y1);
        QPointF p2 = Coord(i.second.x2, i.second.y2);
        QRectF rect(p1, p2);

        if(grid_chosen.at(i.first)) pen.setColor(Qt::darkMagenta);
        painter->setPen(pen);
        painter -> drawRect(rect);
        if(grid_chosen.at(i.first)) pen.setColor(Qt::lightGray);
    }
}

////////////////////////////////////////////////////////////////////////////////
// draw contents

void Detector2DHitItem::DrawEventContent(QPainter *painter)
{
    QPen linepen(Qt::black);
    linepen.setCapStyle(Qt::RoundCap);
    linepen.setWidth(5);
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setPen(linepen);

    // helper
    //auto get_string = [](double x, double y) -> QString
    //{
    //    QString res((std::string("(") + std::to_string((int)x) + std::string(",") + std::to_string((int)y) + std::string(")")).c_str());
    //    return res;
    //};

    UpdateEventContent();

    // draw all hits
    for(auto &i: global_hits) {
        painter->drawPoint(i);

        // draw value
        //painter->save();
        //painter->setFont(QFont("times",8));
        //painter->translate(p.x(), p.y());
        //painter->rotate(45);
        //painter -> drawText(0, 0, get_string(i.x, i.y));
        //painter->restore();
    }

    // draw real hits
    linepen.setColor(Qt::blue);
    painter -> setPen(linepen);
    for(auto &i: real_hits) {
        //painter->drawPoint(p);

        // real hits blue circle
        linepen.setWidth(1);
        painter->setPen(linepen);
        painter->drawEllipse(i, 8.0, 8.0);

        linepen.setWidth(5);
        painter->setPen(linepen);
    }

    // draw fitted hits
    linepen.setColor(Qt::red);
    painter -> setPen(linepen);
    for(auto &i: fitted_hits) {
        //painter->drawPoint(p); // fitted hits only draw circle

        linepen.setWidth(1);
        painter->setPen(linepen);
        painter->drawEllipse(i, 8.0, 8.0);
        linepen.setWidth(5);
        painter->setPen(linepen);
    }

    // draw background hits
    linepen.setColor(Qt::black);
    painter -> setPen(linepen);
    for(auto &i: background_hits) {
        painter->drawPoint(i);
    }
}

////////////////////////////////////////////////////////////////////////////////
// clear

void Detector2DHitItem::Clear()
{
    _title = "";
}

////////////////////////////////////////////////////////////////////////////////
// set detector title

void Detector2DHitItem::SetTitle(const std::string &s)
{
    _title = QString(s.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// set detector x/y strip index range

void Detector2DHitItem::SetDataRange(int x_min, int x_max, int y_min, int y_max)
{
    data_x_min = x_min;
    data_x_max = x_max;
    data_y_min = y_min;
    data_y_max = y_max;
}

////////////////////////////////////////////////////////////////////////////////
// pass detector handle

void Detector2DHitItem::PassDetectorHandle(AbstractDetector *fD)
{
    detector = fD;

    auto d = fD -> GetDimension();

    //SetDataRange(0., d.x, 0., d.y);
    SetDataRange(-d.x/2., d.x/2., -d.y/2., d.y/2.);
}

////////////////////////////////////////////////////////////////////////////////
// update the event for drawing

void Detector2DHitItem::UpdateEventContent()
{
    global_hits.clear();
    real_hits.clear();
    fitted_hits.clear();
    background_hits.clear();

    // draw previous events
    if(counter <= 0) {
        int index = -counter;
        counter = 1; // reset counter for next event
        if(index >= CACHE_EVENT_SIZE) return;
        
        global_hits = global_hits_cache.at(index);
        real_hits = real_hits_cache.at(index);
        fitted_hits = fitted_hits_cache.at(index);
        background_hits = background_hits_cache.at(index);
        return;
    }

    // draw new events
    // all hits
    auto det_hits = detector -> GetGlobalHits();
    for(auto &i: det_hits) {
        QPointF p = Coord(i.x, i.y);
        global_hits.push_back(p);
    }
    global_hits_cache.push_front(global_hits);
    if(global_hits_cache.size() > CACHE_EVENT_SIZE)
        global_hits_cache.pop_back();

    // real hits
    auto det_real_hits = detector -> GetRealHits();
    for(auto &i: det_real_hits) {
        QPointF p = Coord(i.x, i.y);
        real_hits.push_back(p);
    }
    real_hits_cache.push_front(real_hits);
    if(real_hits_cache.size() > CACHE_EVENT_SIZE)
        real_hits_cache.pop_back();

    // draw fitted hits
    auto det_fitted_hits = detector -> GetFittedHits();
    for(auto &i: det_fitted_hits) {
        QPointF p = Coord(i.x, i.y);
        fitted_hits.push_back(p);
    }
    fitted_hits_cache.push_front(fitted_hits);
    if(fitted_hits_cache.size() > CACHE_EVENT_SIZE)
        fitted_hits_cache.pop_back();

    // draw background hits
    auto det_background_hits = detector -> GetBackgroundHits();
    for(auto &i: det_background_hits) {
        QPointF p = Coord(i.x, i.y);
        background_hits.push_back(p);
    }
    background_hits_cache.push_front(background_hits);
    if(background_hits_cache.size() > CACHE_EVENT_SIZE)
        background_hits_cache.pop_back();
}

////////////////////////////////////////////////////////////////////////////////
// setup a event counter for drawing back events

void Detector2DHitItem::SetCounter(int i)
{
    counter = i;
}

};
