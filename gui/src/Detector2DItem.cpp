#include "Detector2DItem.h"
#include "ColorSpectrum.h"
#include <iostream>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// ctor

Detector2DItem::Detector2DItem()
{
    // default bounding rect
    _boundingRect.setRect(0, 0, 50, 50);

    color_spectrum = new ColorSpectrum();
}

////////////////////////////////////////////////////////////////////////////////
// dtor

Detector2DItem::~Detector2DItem()
{
    x_strips.clear();
    y_strips.clear();
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
    float margin_x = ratio_away * default_range.width();
    float margin_y = ratio_away * default_range.height();

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

    // draw axis ticks
    QPointF origin = Coord(data_x_min, data_y_min);
    text_width = fm.width("0  "), text_height = fm.height();
    origin.setX(origin.x() - text_width/2), origin.setY(origin.y() + text_height - 5);

    QPointF max_x = Coord(data_x_max, data_y_min);
    text_width = fm.width(std::to_string((int)data_x_max).c_str()), text_height = fm.height();
    max_x.setX(max_x.x() - text_width/2), max_x.setY(max_x.y() + text_height - 5);

    QPointF max_y = Coord(data_x_min, data_y_max);
    text_width = fm.width(std::to_string((int)data_y_max).c_str()), text_height = fm.height();
    max_y.setX(max_y.x() - text_width);

    if(readout_type.find("UV") == std::string::npos) {
        painter -> drawText(origin, "0 (strip #)");
        painter -> drawText(max_x, std::to_string((int)data_x_max).c_str());
        painter -> drawText(max_y, std::to_string((int)data_y_max).c_str());
    } else {
    }
}

////////////////////////////////////////////////////////////////////////////////
// draw contents

void Detector2DItem::DrawContent(QPainter *painter)
{
    auto && strips = PrepareStrips();

    for(auto &s: strips) {
        painter -> setPen(s.second);
        painter -> drawLine(s.first);
    }

    // draw max adc text
    //if(x_strip_max_adc > 0) {
    //    QPointF x_coord = Coord(x_strip_max_adc_index, data_y_min);
    //    painter -> drawText(x_coord, (std::to_string(x_strip_max_adc) + "ADC").c_str());
    //}
    //if(y_strip_max_adc > 0) {
    //    QPointF y_coord = Coord(data_x_min, y_strip_max_adc_index);
    //    painter -> drawText(y_coord, (std::to_string(y_strip_max_adc) + "ADC").c_str());
    //}
}

////////////////////////////////////////////////////////////////////////////////
// prepare strips to draw

QVector<std::pair<QLineF, QColor>> Detector2DItem::PrepareStrips()
{
    QVector<std::pair<QLineF, QColor>> res;

    // get line equation (two end points) for x/y gem chambers from strip index
    auto line_xy = [&](int strip_index, bool is_x_strip) -> QLineF
    {
        QPointF p1, p2;
        if(is_x_strip) {
            if(strip_index > data_x_max)
                strip_index = data_x_max;
            p1 = Coord(strip_index, data_y_min);
            p2 = Coord(strip_index, data_y_max);
        } else {
            if(strip_index > data_y_max)
                strip_index = data_y_max;
            p1 = Coord(data_x_min, strip_index);
            p2 = Coord(data_x_max, strip_index);
        }

        return QLineF(p1, p2);
    };

    // get line equation (two end points) for u/v gem chambers from strip index
    auto line_uv = [&](int strip_index, bool is_u_strip, float angle) -> QLineF
    {
        QPointF p1, p2;
        float angle_rad = angle * 3.14 / 180. / 2.;
        float distance; // vertical distance to the 0th strip
        float pitch = 1.35; // it should be sqrt(2), however, need to make it smaller to count margins
        float x_intercept = 0, y_intercept = 0;

        if(is_u_strip)
        {
            if(strip_index > data_x_max)
                strip_index = data_x_max;

            distance = strip_index * pitch;
            x_intercept = distance / cos(angle_rad);
            y_intercept = distance / sin(angle_rad);

            if(x_intercept <= data_x_max)
                p1 = Coord(x_intercept, data_y_min);
            else
                p1 = Coord(data_x_max, (x_intercept-data_x_max) / tan(angle_rad));

            if(y_intercept <= data_y_max)
                p2 = Coord(data_x_min, y_intercept);
            else
                p2 = Coord((y_intercept - data_y_max) * tan(angle_rad), data_y_max);

        } else {
            if(strip_index > data_y_max)
                strip_index = data_y_max;

            distance = strip_index * pitch;
            x_intercept = distance / cos(angle_rad);
            y_intercept = distance / sin(angle_rad);

            if(x_intercept <= data_x_max)
                p1 = Coord(data_x_max - x_intercept, data_y_min);
            else
                p1 = Coord(data_x_min,
                        (x_intercept-data_x_max) / tan(angle_rad));

            if(y_intercept <= data_y_max)
                p2 = Coord(data_x_max, y_intercept);
            else
                p2 = Coord(data_x_max - (y_intercept - data_y_max) * tan(angle_rad), data_y_max);
        }

        return QLineF(p1, p2);
    };

    // a helper
    x_strip_max_adc = -9999, y_strip_max_adc = -9999;
    auto process_plane = [&](const std::vector<std::pair<int, float>> &strips,
            const float &angle, bool is_x_strip)
    {
        for(auto &strip: strips) 
        {
            int strip_index = strip.first;
            QLineF l;
            if(angle == 0 || angle == 90)
                l = line_xy(strip_index, is_x_strip);
            else
                l = line_uv(strip_index, is_x_strip, angle);

            // color bar
            float adc = 0;
            if(strip.second >= 800.)
                adc = 1.0;
            else
                adc = (float)strip.second / 800.;
            QColor color = color_spectrum->toColor(adc);

            res.push_back(std::pair<QLineF, QColor>(l, color));

            // for debug color spectrum
            if(is_x_strip) {
                if(x_strip_max_adc < strip.second) {
                    x_strip_max_adc = strip.second;
                    x_strip_max_adc_index = strip.first;
                }
            } else {
                if(y_strip_max_adc < strip.second) {
                    y_strip_max_adc = strip.second;
                    y_strip_max_adc_index = strip.first;
                }
            }
            //
        }
    };

    // process strips
    process_plane(x_strips, x_strip_angle, true);
    process_plane(y_strips, y_strip_angle, false);

    return res;
}

////////////////////////////////////////////////////////////////////////////////
// clear

void Detector2DItem::Clear()
{
    _title = "";
    x_strips.clear();
    y_strips.clear();
}

////////////////////////////////////////////////////////////////////////////////
// set detector title

void Detector2DItem::SetTitle(const std::string &s)
{
    _title = QString(s.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// set detector x/y strip index range

void Detector2DItem::SetStripIndexRange(int x_min, int x_max, int y_min, int y_max)
{
    data_x_min = x_min;
    data_x_max = x_max;
    data_y_min = y_min;
    data_y_max = y_max;
}

////////////////////////////////////////////////////////////////////////////////
// set detector x/y strip angle

void Detector2DItem::SetStripAngle(float x_angle, float y_angle)
{
    x_strip_angle = x_angle;
    y_strip_angle = y_angle;
}
