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
#include <string>

class ColorSpectrum;

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
    void SetStripIndexRange(int x_strip_min, int x_strip_max,
            int y_strip_min, int y_strip_max);
    void SetStripAngle(float x_angle, float y_angle);
    void SetReadoutType(const std::string &s){readout_type = s;}

    // getters
    const std::string &GetReadoutType() const {return readout_type;}

    // receive data to plot
    template<typename T1, typename T2>
    void ReceiveContents(const std::vector<std::pair<T1, T2>> &_x_strips,
                         const std::vector<std::pair<T1, T2>> &_y_strips)
    {
        x_strips.clear();
        y_strips.clear();

        for(auto &i: _x_strips) {
            x_strips.emplace_back(static_cast<int>(i.first), static_cast<float>(i.second));
        }

        for(auto &i: _y_strips) {
            y_strips.emplace_back(static_cast<int>(i.first), static_cast<float>(i.second));
        }
    }

protected:
    QVector<std::pair<QLineF, QColor>> PrepareStrips();
    void UpdateDrawingRange();
    void DrawAxis(QPainter *painter);
    void DrawContent(QPainter *painter);
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
    std::vector<std::pair<int, float>> x_strips; // <strip_no, adc>
    std::vector<std::pair<int, float>> y_strips; // <strip_no, adc>

    QString _title = QString("detector 0");
    std::string readout_type = "CARTESIAN";

    // data range
    float data_x_min=0, data_x_max=100, data_y_min=0, data_y_max=100;
    float x_strip_angle = 0, y_strip_angle = 90;
    // drawing range
    float area_x1, area_x2, area_y1, area_y2;

    // convert value to rgb color
    ColorSpectrum *color_spectrum;

    // for debug color spectrum only
    int x_strip_max_adc, x_strip_max_adc_index;
    int y_strip_max_adc, y_strip_max_adc_index;
};

#endif
