#include "Detector2DItem.h"
#include "AbstractDetector.h"
#include <iostream>
#include <cmath>

namespace tracking_dev {

////////////////////////////////////////////////////////////////////////////////
// ctor

Detector2DItem::Detector2DItem()
{
    // default bounding rect
    _boundingRect.setRect(0, 0, 50, 50);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

Detector2DItem::~Detector2DItem()
{
}

////////////////////////////////////////////////////////////////////////////////
// bounding rect

QRectF Detector2DItem::boundingRect() const
{
    return _boundingRect;
}

////////////////////////////////////////////////////////////////////////////////
// paint

void Detector2DItem::paint(QPainter *painter,
        [[maybe_unused]] const QStyleOptionGraphicsItem *option,
        [[maybe_unused]] QWidget *widget)
{
    UpdateDrawingRange();

    // draw grids
    DrawGrids(painter);

    // draw a frame
    DrawAxis(painter);

    // draw contents
    DrawContent(painter);
}

////////////////////////////////////////////////////////////////////////////////
// resize event

void Detector2DItem::resizeEvent()
{
    QPainter painter;
    paint(&painter);
}

////////////////////////////////////////////////////////////////////////////////
// set bounding rect

void Detector2DItem::SetBoundingRect(const QRectF &f)
{
    prepareGeometryChange(); // only needed when bounding rect changes

    _boundingRect = mapRectFromScene(f);
}

////////////////////////////////////////////////////////////////////////////////
// update range: update the drawing area range

void Detector2DItem::UpdateDrawingRange()
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

void Detector2DItem::DrawAxis(QPainter *painter)
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
    int text_width = fm.width(_title), text_height = fm.height();
    int x_text = (area_x1 + area_x2) / 2 - text_width / 2, 
        y_text = area_y1 - text_height / 2;
    painter -> drawText(x_text, y_text, _title);
}

////////////////////////////////////////////////////////////////////////////////
// draw detector grids

void Detector2DItem::DrawGrids(QPainter *painter)
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

void Detector2DItem::DrawContent(QPainter *painter)
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

    // draw all hits
    auto hits = detector -> GetGlobalHits();
    for(auto &i: hits) {
        QPointF p = Coord(i.x, i.y);
        painter->drawPoint(p);

        // draw value
        //painter->save();
        //painter->setFont(QFont("times",8));
        //painter->translate(p.x(), p.y());
        //painter->rotate(45);
        //painter -> drawText(0, 0, get_string(i.x, i.y));
        //painter->restore();
    }

    // draw real hits
    auto real_hits = detector -> GetRealHits();
    linepen.setColor(Qt::blue);
    painter -> setPen(linepen);
    for(auto &i: real_hits) {
        QPointF p = Coord(i.x, i.y);
        //painter->drawPoint(p);

        // real hits blue circle
        linepen.setWidth(1);
        painter->setPen(linepen);
        painter->drawEllipse(p, 8.0, 8.0);

        linepen.setWidth(5);
        painter->setPen(linepen);
    }

    // draw fitted hits
    auto fitted_hits = detector -> GetFittedHits();
    linepen.setColor(Qt::red);
    painter -> setPen(linepen);
    for(auto &i: fitted_hits) {
        QPointF p = Coord(i.x, i.y);
        //painter->drawPoint(p); // fitted hits only draw circle

        linepen.setWidth(1);
        painter->setPen(linepen);
        painter->drawEllipse(p, 8.0, 8.0);
        linepen.setWidth(5);
        painter->setPen(linepen);
    }

    // draw background hits
    auto background_hits = detector -> GetBackgroundHits();
    linepen.setColor(Qt::black);
    painter -> setPen(linepen);
    for(auto &i: background_hits) {
        QPointF p = Coord(i.x, i.y);
        painter->drawPoint(p);
    }
}

////////////////////////////////////////////////////////////////////////////////
// clear

void Detector2DItem::Clear()
{
    _title = "";
}

////////////////////////////////////////////////////////////////////////////////
// set detector title

void Detector2DItem::SetTitle(const std::string &s)
{
    _title = QString(s.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// set detector x/y strip index range

void Detector2DItem::SetDataRange(int x_min, int x_max, int y_min, int y_max)
{
    data_x_min = x_min;
    data_x_max = x_max;
    data_y_min = y_min;
    data_y_max = y_max;
}

////////////////////////////////////////////////////////////////////////////////
// pass detector handle

void Detector2DItem::PassDetectorHandle(AbstractDetector *fD)
{
    detector = fD;

    auto d = fD -> GetDimension();

    //SetDataRange(0., d.x, 0., d.y);
    SetDataRange(-d.x/2., d.x/2., -d.y/2., d.y/2.);
}

};
