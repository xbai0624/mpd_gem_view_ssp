#include "HistoItem.h"

#include <cmath>
#include <QPen>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

// for unit test
static const double test_dist[] = {
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0, 
    0,      0,      0,      0,      0,      0,      0,      0,      1,      0, 
    0,      8,      5,     21,     20,     44,    116,    216,    305,    563,
    911,   1294,   1914,   2648,   3435,   4407,   5321,   6259,   7153,   7498,
    7932,   7869,   7657,   7031,   6216,   5379,   4288,   3435,   2588,   1871,
    1326,    881,    533,    363,    210,    122,     67,     40,     30,     15,
    6,      1,      0,      1,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
};

HistoItem::HistoItem()
{
    bounding_rect = QRectF(0, 0, 100, 100);
    updateDrawingRange();
    clearContent();
}

HistoItem::HistoItem(double width, double height)
{
    bounding_rect = QRectF(0, 0, width, height);
    updateDrawingRange();
    clearContent();
}

HistoItem::~HistoItem()
{
}

void HistoItem::updateDrawingRange()
{
    // total drawable area - leave a margin to get rid of scroll bars
    double margin = 0.1;
    drawing_range = QRectF(
            bounding_rect.width() * margin, bounding_rect.height() * margin,
            bounding_rect.width() * (1 - 2*margin), bounding_rect.height() * (1 - 2*margin)
            );
}

void HistoItem::clearContent()
{
    original_data2.clear();
    _data2.clear();
    _content.clear();
    _select_box.clear();
    _coordinate_frame.clear();
}

void HistoItem::SetBoundingRect(const QRectF &f)
{
    prepareGeometryChange();

    bounding_rect = mapRectFromScene(f);
    //bounding_rect = f;
    bounding_rect.setRect(0, 0, bounding_rect.width(), bounding_rect.height());

    setPos(f.x(), f.y());

    updateDrawingRange();
}

QRectF HistoItem::boundingRect() const
{
    // we don't want the bounding rectangle to change, that
    // will make the drawing area change on different data.
    // so we make a fixed drawing area, and scale our data to
    // this area

    return bounding_rect;
}

void HistoItem::PassData()
{
    original_data2.clear();

    for(int i=0; i<100; i++) {
        original_data2.push_back(qMakePair(i, test_dist[i]));
    }

    updateDrawingContent();
}

void HistoItem::updateDrawingContent()
{
    // clear previous event, drawing content is saved in _data2
    _data2.clear();

    // get data range for zoom in
    QPair<double, double> draw_data_range;
    if(second_pos > first_pos) {
        draw_data_range.first = data_x_range.first +
            (first_pos - drawing_range.x()) / drawing_range.width() * (data_x_range.second - data_x_range.first);
        draw_data_range.second = data_x_range.first +
            (second_pos - drawing_range.x()) / drawing_range.width() * (data_x_range.second - data_x_range.first);
        //qDebug()<<"data range selected" << draw_data_range.first<<", "<<draw_data_range.second;
    }

    // extract drawable-part data
    for(auto &i: original_data2) {
        if(second_pos > first_pos) {
            if(i.first >= draw_data_range.first && i.first <= draw_data_range.second)
                _data2.push_back(i);
        }
        else
            _data2.push_back(i);
    }

    prepareDataShape();
}

void HistoItem::prepareDataShape()
{
    updateDrawingRange();

    // shape data
    auto find_data_range = [&](QPair<double, double> &xr, QPair<double, double> &yr){
        if(_data2.size() <= 0)
            xr.first = 0, xr.second = 0, yr.first = 0, yr.second = 0;

        xr.first = xr.second = _data2.at(0).first;
        yr.first = yr.second = _data2.at(0).second;

        for(auto &i: _data2)
        {
            double x = i.first, y = i.second;

            if(x < xr.first)
                xr.first = x;
            if(x > xr.second)
                xr.second = x;
            if(y < yr.first)
                yr.first = y;
            if(y > yr.second)
                yr.second = y;
        }
    };

    if(_data2.size() > 0) {
        _content.clear();

        find_data_range(data_x_range, data_y_range);

        double scale_factor_x = drawing_range.width()/(data_x_range.second - data_x_range.first);
        double scale_factor_y = drawing_range.height()/(data_y_range.second - data_y_range.first);

        _content<<QPointF(drawing_range.x(), drawing_range.y() + drawing_range.height());

        for(int i=0; i<_data2.size()-1; i++)
        {
            _content<<QPointF(
                    drawing_range.x() + (_data2[i].first - data_x_range.first)*scale_factor_x, 
                    drawing_range.y() + drawing_range.height() - (_data2[i].second - data_y_range.first)*scale_factor_y
                    )
                <<QPointF(
                        drawing_range.x() + (_data2[i+1].first - data_x_range.first)*scale_factor_x, 
                        drawing_range.y() + drawing_range.height() - (_data2[i].second - data_y_range.first)*scale_factor_y
                        );
        }

        _content << QPointF(drawing_range.x() + (data_x_range.second - data_x_range.first)*scale_factor_x, drawing_range.y() + drawing_range.height());
    }
}

void HistoItem::paint(QPainter *painter,
        [[maybe_unused]] const QStyleOptionGraphicsItem *option,
        [[maybe_unused]] QWidget *widget)
{
    // scale pen width
    qreal scaleX = painter -> worldTransform().m11();
    qreal scaleY = painter -> worldTransform().m22();
    qreal scale = std::sqrt(scaleX * scaleY);

    //QPen pen1(Qt::blue, 2./scale);
    QPen pen1(Qt::blue);

    // draw data
    painter -> setPen(pen1);
    prepareDataShape();
    painter -> drawPolygon(_content);

    pen1.setColor(Qt::black);
    painter -> setPen(pen1);
    // draw coordinate frame
    drawAxis(painter, data_x_range.first, data_x_range.second, data_y_range.first, data_y_range.second);

    // draw the bounding rect
    pen1.setColor(QColor(189, 189, 189));
    pen1.setWidthF(2./scale);
    painter -> setPen(pen1);

    //drawBoundingRect(painter);

    // draw the select box
    if(_select_box.size() == 4)
    {
        pen1.setColor(QColor(138, 206, 247));
        pen1.setWidthF(2./scale);
        painter -> setPen(pen1);

        drawSelectBox(painter);
    }

    // draw a title
    drawTitle(painter);
}

void HistoItem::drawAxis(QPainter *painter, double data_x1, double data_x2, double data_y1, double data_y2)
{
    _coordinate_frame.clear();
    _coordinate_frame << QPointF(drawing_range.x(), drawing_range.y()*0.6)
        <<QPointF(drawing_range.x(), drawing_range.y() + drawing_range.height())
        <<QPointF(drawing_range.x() + drawing_range.width(), drawing_range.y() + drawing_range.height())
        <<QPointF(drawing_range.x() + drawing_range.width(), drawing_range.y()*0.6);

    painter -> drawPolygon(_coordinate_frame);

    // draw ticks, length in pixel
    double tick_length = bounding_rect.width() / 100;
    // text font below tick
    QFont font;
    int font_size = bounding_rect.width() / 50 > 6 ? bounding_rect.width() / 50 : 6;
    font.setPixelSize(font_size);
    QFontMetrics fm(font);
    painter -> setFont(font);

    int decimal_digits = 0; // show how many digits after decimal point

    // only draw meaningful settings
    if(data_x2 > data_x1) {
        double ratio = drawing_range.width() / (data_x2 - data_x1);
        QVector<double> ticks = getAxisTicks(data_x1, data_x2);

        for(auto &i: ticks) {
            QPointF p1(drawing_range.x() + (i-data_x1) * ratio, drawing_range.y() + drawing_range.height());
            QPointF p2(drawing_range.x() + (i-data_x1) * ratio, drawing_range.y() + drawing_range.height() + tick_length);
            painter -> drawLine(p1, p2);

            QString s(std::to_string(i).c_str());
            s = s.mid(0, s.lastIndexOf(".") + decimal_digits);
            int font_width = fm.horizontalAdvance(s);
            int font_height = fm.height();

            painter -> drawText(drawing_range.x() + (i-data_x1)*ratio - font_width / 2,
                    drawing_range.y() + drawing_range.height() + font_height * 1.2,
                    s);
        }
    }

    // only draw meaningful settings
    if(data_y2 > data_y1) {
        double ratio = (drawing_range.height()) / (data_y2 - data_y1);
        QVector<double> ticks = getAxisTicks(data_y1, data_y2);

        for(auto &i: ticks) {
            QPointF p1(drawing_range.x() - tick_length, drawing_range.y() + drawing_range.height() - (i-data_y1) * ratio);
            QPointF p2(drawing_range.x(), drawing_range.y() + drawing_range.height() - (i-data_y1) * ratio);
            painter -> drawLine(p1, p2);

            QString s(std::to_string(i).c_str());
            s = s.mid(0, s.lastIndexOf(".") + decimal_digits);
            int font_width = fm.horizontalAdvance(s);
            int font_height = fm.height();

            painter -> drawText(drawing_range.x() - tick_length - font_width,
                    drawing_range.y() + drawing_range.height() - (i - data_y1)*ratio + font_height/2,
                    s);
        }
    }
}

void HistoItem::drawBoundingRect(QPainter *painter)
{
    QPolygonF frame;
    frame << QPointF(bounding_rect.x(), bounding_rect.y())
        <<QPointF(bounding_rect.x(), bounding_rect.y() + bounding_rect.height())
        <<QPointF(bounding_rect.x() + bounding_rect.width(), bounding_rect.y() + bounding_rect.height())
        <<QPointF(bounding_rect.x() + bounding_rect.width(), bounding_rect.y());

    painter -> drawPolygon(frame);
}

void HistoItem::drawSelectBox(QPainter *painter)
{
    painter -> drawPolygon(_select_box);
}

void HistoItem::mousePressEvent([[maybe_unused]]QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "histo item mouse press event.";

    if(event -> button() == Qt::LeftButton){
        mouse_left_button_down = true;
        first_pos = mapFromScene(event -> scenePos()).x();
    }

    //qDebug() << "first pos: "<<first_pos;
}

void HistoItem::mouseReleaseEvent([[maybe_unused]]QGraphicsSceneMouseEvent *event)
{
    //qDebug() << " histo item mouse release event.";

    if(event -> button() == Qt::LeftButton) {
        mouse_left_button_down = false;
        second_pos = mapFromScene(event -> scenePos()).x();

        _select_box.clear();
    }
    else if(event -> button() == Qt::RightButton) {
        first_pos = 0, second_pos = 0;
    }

    updateDrawingContent();

    update();
    //qDebug() << "second pos: "<<second_pos;
}

void HistoItem::mouseMoveEvent([[maybe_unused]]QGraphicsSceneMouseEvent *event)
{
    if(!mouse_left_button_down)
        return;

    _select_box.clear();
    qreal current_pos = mapFromScene(event -> scenePos()).x();

    _select_box << QPointF(bounding_rect.x() + first_pos, bounding_rect.y())
        << QPointF(bounding_rect.x() + first_pos, bounding_rect.y() + bounding_rect.height())
        << QPointF(bounding_rect.x() + current_pos, bounding_rect.y() + bounding_rect.height())
        << QPointF(bounding_rect.x() + current_pos, bounding_rect.y());

    update();
}

QVector<double> HistoItem::getAxisTicks(double a_min, double a_max)
{
    QVector<double> marks;

    if(a_max <= a_min)  {
        return marks;
    }
    // 1. get the total range
    double distance = a_max - a_min;

    // 2. normalize the range to: 1 - 10
    int power = 0;
    if(distance > 10) {
        while(distance > 10.)
        {
            distance /= 10;
            power++;
        }
    }
    else if(distance < 1) {
        while(distance < 1) {
            distance *= 10;
            power--;
        }
    }

    // 3. get scale
    auto pow_span = [](double a, int n) -> double
    {
        if(n == 0) 
            return 1.0;

        if( n > 0)
            return a * pow(a, n-1);

        return 1./a * pow(a, n+1);
    };
    double scale = pow_span(10, power);

    // 4. get proper space, in units: 1, or 0.5, or 0.25
    double spacing = 1.0;
    if(distance >= 5)
        spacing = 1.0;
    else if(distance >= 2)
        spacing /= 2.0;
    else
        spacing /= 4.0;

    spacing *= scale;

    // 5. generate marks
    int n = 1.0;
    if(a_min > 0) {
        // range on the right side of x=0
        while(n * spacing <= a_max) {
            if(n*spacing >= a_min)
                marks.push_back(n*spacing);
            n++;
        }
    }
    else if(a_max < 0) {
        // range on the left side of x=0
        while (n*spacing <= -a_min) {
            if(n*spacing >= -a_max)
                marks.push_back(-n*spacing);
            n++;
        }
    }
    else {
        // range spans x = 0
        marks.push_back(0);
        while(n * spacing <= a_max) {
            marks.push_back(n*spacing);
            n++;
        }
        n = 1.0;
        while(n*spacing <= -a_min) {
            marks.push_back(-n*spacing);
            n++;
        }
    }

    return marks;
}

void HistoItem::SetTitle(const std::string &s)
{
    _title = QString::fromStdString(s);
}

void HistoItem::drawTitle(QPainter *painter)
{
    // text font below tick
    QFont font;
    int font_size = bounding_rect.width() / 50 > 6 ? bounding_rect.width() / 50 : 6;
    font.setPixelSize(font_size);
    QFontMetrics fm(font);
    painter -> setFont(font);

    int font_width = fm.horizontalAdvance(_title);
    int font_height = fm.height();

    QPen pen1(Qt::black);
    // draw data
    painter -> setPen(pen1);
 
    painter -> drawText(drawing_range.x() + drawing_range.width()/2 - font_width/2,
            drawing_range.y() - font_height/2 - 2, // 2 pixel to give it some margin
            _title);
}
