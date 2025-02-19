/*******************************************************************************
 * A qt replacement for histograms
 *     it accepts two types of data: 1) a vector of double; 2) a vector of QPair
 *     For the 1st case, the vector consists of only y values, 
 *     x will be default to: from 1 to the max length of the vector
 *     For the 2nd case, x axis is the QPair.first, y axis is the QPair.second
 *     Xinzhan Bai 02/03/2024
 ******************************************************************************/


#ifndef HISTOITEM_H
#define HISTOITEM_H

#include <QGraphicsItem>
#include <QVector>

class HistoItem : public QGraphicsItem {
public:
    HistoItem();
    HistoItem(double width, double height);
    ~HistoItem();

    QRectF boundingRect() const;
    void SetBoundingRect(const QRectF& f);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem *option = nullptr,
            QWidget *widget = nullptr);

    // paint helper
    void drawAxis(QPainter *painter, double data_x1=0, double data_x2=0, double data_y1=0, double data_y2=0);
    void drawBoundingRect(QPainter *painter);
    void drawSelectBox(QPainter *painter);

    template<typename T> void ReceiveContents(const std::vector<T> &v) {
        original_data2.clear();

        for(size_t i=0; i<v.size(); i++) {
            original_data2.push_back(qMakePair(static_cast<double>(i), static_cast<double>(v.at(i))));
        }

        updateDrawingContent();
    }
    void PassData();
    void updateDrawingRange();
    void updateDrawingContent();
    void prepareDataShape();
    void clearContent();
    void SetTitle(const std::string &);
    void drawTitle(QPainter *painter);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    QVector<double> getAxisTicks(double a_min, double a_max);

private:
    // reserve a copy of raw data get passed
    QVector<QPair<double, double>> original_data2;
    // data for drawing
    QVector<QPair<double, double>> _data2;
    // get the max and min of data, for drawing axis
    QPair<double, double> data_x_range, data_y_range;

    QPolygonF _content;
    QPolygonF _coordinate_frame;

    QRectF bounding_rect;
    // make the actual drawing area smaller to remove scroll bars
    QRectF drawing_range;

    // for zoom operation using left click
    bool mouse_left_button_down = false;
    qreal first_pos=0, second_pos=0;
    QPolygonF _select_box;

    // title
    QString _title = "hist";
};

#endif
