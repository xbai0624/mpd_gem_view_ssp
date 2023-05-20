////////////////////////////////////////////////////////////////////////////////
// A class to show fired GEM detector 2D strips                               //
////////////////////////////////////////////////////////////////////////////////

#ifndef DETECTOR_2D_ITEM_H
#define DETECTOR_2D_ITEM_H

#include <QGraphicsItem>
#include <QRectF>
#include <QPolygonF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <vector>
#include <deque>
#include <string>
#include <iostream>

namespace tracking_dev{

#define CACHE_EVENT_SIZE 100
class AbstractDetector;

class Detector2DItem : public QGraphicsItem
{
public:
    Detector2DItem();
    ~Detector2DItem();

    // memebers
    QRectF boundingRect() const;
    void paint(QPainter *painter,
            const QStyleOptionGraphicsItem *option = nullptr, QWidget *widget = nullptr);
    virtual void resizeEvent();

    // setters
    void SetBoundingRect(const QRectF &f);
    void SetTitle(const std::string &s);

    // pass detector pointer to be plotted
    void PassDetectorHandle(AbstractDetector *fD);

    void SetDataRange(int x_min, int x_max, int y_min, int y_max);
    void SetCounter(int i);

protected:
    void UpdateDrawingRange();
    void UpdateEventContent();
    void DrawAxis(QPainter *painter);
    void DrawEventContent(QPainter *painter);
    void DrawGrids(QPainter *painter);
    void Clear();

    // convert logical coord (data) to QGraphicsItem coord (drawing)
    template<typename T1, typename T2>
    QPointF Coord(const T1& _x, const T2& _y)
    {
        float x = static_cast<float>(_x);
        float y = static_cast<float>(_y);

        float x_draw = area_x1 + 
            (x - data_x_min) / (data_x_max - data_x_min) * (area_x2 - area_x1);
        float y_draw = 
            (y - data_y_min) / (data_y_max - data_y_min) * (area_y2 - area_y1);

        // invert y axis
        y_draw = area_y1 + (area_y2 - area_y1) - y_draw;

        return QPointF(x_draw, y_draw);
    }

private:
    QRectF _boundingRect;

    QString _title = QString("detector 0");

    // data range
    float data_x_min=0, data_x_max=100, data_y_min=0, data_y_max=100;
    // drawing range
    float area_x1, area_x2, area_y1, area_y2;
    // drawing area margin - distance away from bounding rect
    float margin_x, margin_y;

    AbstractDetector *detector;

    // current event to draw
    std::vector<QPointF> global_hits;
    std::vector<QPointF> real_hits;
    std::vector<QPointF> fitted_hits;
    std::vector<QPointF> background_hits;

    // cache events for drawing purpose
    std::deque<std::vector<QPointF>> global_hits_cache;
    std::deque<std::vector<QPointF>> real_hits_cache;
    std::deque<std::vector<QPointF>> fitted_hits_cache;
    std::deque<std::vector<QPointF>> background_hits_cache;

    // forward step size
    int counter = 1;
};

};
#endif
