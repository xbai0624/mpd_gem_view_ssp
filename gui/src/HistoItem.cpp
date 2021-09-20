#include "HistoItem.h"

#include <iostream>

////////////////////////////////////////////////////////////////////////////////
// ctor

HistoItem::HistoItem()
{
    // default bounding rect
    _boundingRect.setRect(0, 0, 50, 50);
}

////////////////////////////////////////////////////////////////////////////////
// dtor

HistoItem::~HistoItem()
{
    _contents.clear();
}

////////////////////////////////////////////////////////////////////////////////
// get bounding rect

QRectF HistoItem::boundingRect() const
{
    return _boundingRect;
}

////////////////////////////////////////////////////////////////////////////////
// mouse press event

void HistoItem::mousePressEvent([[maybe_unused]]QGraphicsSceneMouseEvent *e)
{
    if(root_canvas != nullptr)
        root_canvas -> DrawCanvas(_title.toStdString(), _contents);
    else {
        std::cout<<"Info::QMainCanvas not set yet..."<<std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// set QMainCanvas

void HistoItem::PassQMainCanvasPointer(QMainCanvas *canvas)
{
    root_canvas = canvas;
}

////////////////////////////////////////////////////////////////////////////////
// paint

void HistoItem::paint(QPainter* painter, 
        [[maybe_unused]]const QStyleOptionGraphicsItem *option, 
        [[maybe_unused]]QWidget *widget)
{
    UpdateRange();

    // draw contents
    QPen pen1(Qt::blue, 1);
    painter -> setPen(pen1);
    QPolygonF _shape = PrepareContentShape();
    painter -> drawPolygon(_shape);

    // draw axis
    QPen pen(Qt::black, 1);
    painter -> setPen(pen);
    QVector<QLineF> axis = PrepareAxis();
    painter -> drawLines(axis);

    // draw axis marks
    DrawAxisMarks(painter);
}

////////////////////////////////////////////////////////////////////////////////
// resize event

void HistoItem::resizeEvent()
{
    prepareGeometryChange();
}

////////////////////////////////////////////////////////////////////////////////
// set bounding rect

void HistoItem::SetBoundingRect(const QRectF& f)
{
    prepareGeometryChange();

    // in our case scene coords overlaps with item coords
    _boundingRect = mapRectFromScene(f);
    //_boundingRect = f;
}

////////////////////////////////////////////////////////////////////////////////
// draw histo

QPolygonF HistoItem::PrepareContentShape()
{
    QPolygonF shape;
    if(_contents.size() <= 0) {
        shape << QPointF(0, 0);
        return shape;
    }

    int size = static_cast<int>(_contents.size());

    int start = 0;
    shape << Coord(start, 0);
    for(int i=start;i<size+start;i++) {
        shape << Coord(i, _contents[i-start]);
        shape << Coord(i+1, _contents[i-start]);
    }
    shape << Coord(size+start, _contents[size-1]);
    // close shape
    shape << Coord(size+start, 0);

    return shape;
}

////////////////////////////////////////////////////////////////////////////////
// draw axis

QVector<QLineF> HistoItem::PrepareAxis()
{
    QVector<QLineF> res;

    // main axis
    QLineF x_top(area_x1, area_y1, area_x2, area_y1);
    QLineF x_bot(area_x1, area_y2, area_x2, area_y2);
    QLineF y_left(area_x1, area_y1, area_x1, area_y2);
    QLineF y_right(area_x2, area_y1, area_x2, area_y2);

    res.push_back(x_top);
    res.push_back(x_bot);
    res.push_back(y_left);
    res.push_back(y_right);

    return res;
}

////////////////////////////////////////////////////////////////////////////////
// draw axis marks (a helper)

void HistoItem::DrawAxisMarks(QPainter *painter)
{
    // find optimal mark interval
    auto find_interval = [&](const float &min, const float &max) -> float
    {
        float res;
        // draw 10 major marks
        res = (max - min) / 10;
        return res;
    };

    // generate mark label
    auto label = [&](const float &data_min, const float &data_max, 
            const float &draw_min, const float &draw_max,
            const float &draw_pos, bool invert = false) -> QString
    {
        float data_pos = 0.;
        if(!invert) 
            data_pos = (draw_pos - draw_min) / (draw_max - draw_min) 
                * (data_max - data_min);
        else {
            data_pos = (draw_pos - draw_min) / (draw_max - draw_min) 
                * (data_max - data_min);
            data_pos = data_max - data_pos;
        }

        int ss = static_cast<int>(data_pos);

        return QString(std::to_string(ss).c_str());
    };

    // get text dimension
    QFont font("times", 8);
    QFontMetrics fm(font);
    painter -> setFont(QFont("times", 8));

    // x axis marks
    float x_mark_len = 1.5/100. * (area_x2 - area_x1); // mark length
    float x_pos = area_x1;
    float i = 0;
    float x_w = find_interval(area_x1, area_x2);
    while(x_pos < area_x2)
    {
        painter->drawLine(QLineF(x_pos, area_y2, x_pos, area_y2 - x_mark_len));
        if(i!=0) {
            QString ss = label(data_x_min, data_x_max, area_x1, area_x2, x_pos);
            int font_width = fm.width(ss);
            int font_height = fm.height();
            painter -> drawText(x_pos - font_width/2, area_y2 + font_height, ss);
        }
        x_pos = area_x1 + x_w * (i + 1.0);
        i = i + 1.0;
    }

    // y axis marks
    float y_mark_len = 1.5/100 * (area_y2 - area_y1);
    float y_pos = area_y1;
    i = 0;
    float y_w = find_interval(area_y1, area_y2);
    while(y_pos < area_y2)
    {
        painter -> drawLine(QLineF(area_x1, y_pos, area_x1 + y_mark_len, y_pos));
        if(i != 0) {
            QString ss = label(data_y_min, data_y_max, area_y1, area_y2, y_pos, true);
            int font_width = fm.width(ss);
            int font_height = fm.height();
            painter -> drawText(area_x1 - font_width -2, y_pos + font_height/2, ss);
        }
        y_pos = area_y1 + y_w * (i + 1.0);
        i = i + 1.0;
    }

    // draw title
    int font_width = fm.width(_title);
    float x_title =  (area_x1 + area_x2) / 2. - font_width / 2.;
    //float y_title = area_y1 + fm.height();
    float y_title = area_y1 - 2; // 2 pixel away
    painter -> drawText(x_title, y_title, _title);
}

////////////////////////////////////////////////////////////////////////////////
// How to draw a histogram
//    1) find the dimension information of the drawing area (x1, y1, x2, y2)
//    2) find the range of data, including a) how many bins in x axis (0, x_max)
//                                         b) y_min and y_max
//    3) drawing area (x2 - x1) would be mapped to data range (x_max - 0)
//       drawing area (y2 - y1) would be mapped to data range (y_max - y_min)
//    4) for a point in data: (x_data, y_data) the mapped coords in the drawing 
//       area would be:   x_draw = x1 + (x_data - 0)/x_max * (x2 - x1)
//                        y_draw = y1 + (y_data - y_min) / (y_max - y_min) * (y2 - y1)
// 
// a helper for fixing ranges

void HistoItem::UpdateRange()
{
    QRectF default_range = boundingRect();

    // QGraphicsItem drawing area range
    // 10% pixel away from the bounding rect
    float ratio_away = 0.1;
    float margin_x = ratio_away*default_range.width(); 
    float margin_y = ratio_away*default_range.height(); 
 
    area_x1 = default_range.x() + margin_x;
    area_x2 = area_x1 + default_range.width() - 2.*margin_x;
    area_y1 = default_range.y() + margin_y;
    area_y2 = area_y1 + default_range.height() - 2.*margin_y;

    // set default data range equal to the size of drawing area
    data_x_min = 0;
    data_x_max = area_x2 - area_x1;
    data_y_min = 0;
    data_y_max = area_y2 - area_y1;

    if(_contents.size() <= 0)
       return; 

    // range from data
    // max x is the total counts of x bins
    data_x_max = static_cast<float>(_contents.size());
    float min = _contents[0];
    float max = min;
    for(auto &i: _contents)
    {
        if(min > i) min = i;
        if(max < i) max = i;
    }

    // make sure 0 is included in y axis
    if(min > 0) min = 0;

    data_y_min = min;

    // y max should be 10% larger for pretty reason
    data_y_max = max * (1.1);
}

////////////////////////////////////////////////////////////////////////////////
// 

void HistoItem::SetTitle(const std::string  &ss)
{
    _title = QString(ss.c_str());
}


////////////////////////////////////////////////////////////////////////////////
// 

void HistoItem::Clear()
{
    _title = "";
    _contents.clear();
}
