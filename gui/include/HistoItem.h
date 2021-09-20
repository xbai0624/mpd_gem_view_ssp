////////////////////////////////////////////////////////////////////////////////
//  A class to replace ROOT histograms in qt                                  //
//  (showing ROOT Histograms in qt is too slow)                               //
////////////////////////////////////////////////////////////////////////////////

#ifndef HISTO_ITEM_H
#define HISTO_ITEM_H

#include <QGraphicsItem>
#include <QRectF>
#include <QPolygonF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <vector>

#include "QMainCanvas.h"

class HistoItem : public QGraphicsItem
{
public:
    HistoItem();
    ~HistoItem();

    QRectF boundingRect() const;

    void paint(QPainter* painter, 
            const QStyleOptionGraphicsItem *option=nullptr, QWidget *widget=nullptr);

    virtual void resizeEvent();
    void SetBoundingRect(const QRectF &f);

    // receive data to plot
    template<typename T>
        void ReceiveContents(const std::vector<T> & v) 
        {
            // clear last drawing
            _contents.clear();

            for(auto &i: v) {
                _contents.push_back(static_cast<float>(i));
            }
        }

    // convert logical (data) coordinates to QGraphicsItem (drawing) coordinates
    template<typename T1, typename T2>
        QPointF Coord(const T1 & _x, const T2& _y) 
        {
            float x = static_cast<float>(_x);
            float y = static_cast<float>(_y);

            float x_draw = area_x1 + 
                (x - data_x_min) / (data_x_max - data_x_min) * (area_x2 - area_x1);

            float y_draw = 
                (y - data_y_min) / (data_y_max - data_y_min) * (area_y2 - area_y1);

            y_draw = area_y1 + (area_y2 - area_y1) - y_draw;

            return QPointF(x_draw, y_draw);
        }

    QPolygonF PrepareContentShape();
    QVector<QLineF> PrepareAxis();
    void UpdateRange();

    void PassQMainCanvasPointer(QMainCanvas*);
    void Clear();

    // a helper
    void DrawAxisMarks(QPainter *painter);
    void SetTitle(const std::string &s);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e);

private:
    QRectF _boundingRect;
    std::vector<float> _contents;

    QString _title= QString("hist");

    // connect a QMainCanvas, when this QGraphicsItem
    // got clicked, send its content to QMainCanvas
    QMainCanvas *root_canvas = nullptr;

    // data range
    float data_x_min, data_x_max, data_y_min, data_y_max;
    // drawing range
    float area_x1, area_x2, area_y1, area_y2;
};

#endif
