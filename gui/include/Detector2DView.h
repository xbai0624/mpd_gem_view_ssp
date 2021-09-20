////////////////////////////////////////////////////////////////////////////////
// Detector2DView is an integrated widget, this widget has a QGraphicsView    //
// and a QGraphicsScene member, it organizes QGraphicsItem into any layout    //
// one would like to have.                                                    //
//                                                                            //
// In order to simplify your main GUI interface design:                       //
// One should put all you detectors into this class, and then insert this     //
// class to your main viewer interface                                        //
// Xinzhan Bai, 09/01/2021                                                    //
////////////////////////////////////////////////////////////////////////////////

#ifndef DETECTOR_2D_VIEW_H
#define DETECTOR_2D_VIEW_H

#include "Detector2DItem.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <unordered_map>

#define MaxChamberPerLayer 4

////////////////////////////////////////////////////////////////////////////////
// define an address for layout

struct Detector2DAddress
{
    int layer;
    int pos;

    // default constuctor
    Detector2DAddress():
        layer(0), pos(0)
    {}

    // ctor
    Detector2DAddress(int l, int p):
        layer(l), pos(p)
    {}

    // copy constructor
    Detector2DAddress(const Detector2DAddress &a):
        layer(a.layer), pos(a.pos)
    {}

    // copy assignment
    Detector2DAddress& operator=(const Detector2DAddress &a)
    {
        layer = a.layer; pos = a.pos;
        return *this;
    }

    //
    bool operator==(const Detector2DAddress &a) const
    {
        return (a.layer == layer) && (a.pos == pos);
    }
};

// add a hash function for Detector2DAddress
namespace std {
    template<> struct hash<Detector2DAddress>
    {
        std::size_t operator()(const Detector2DAddress &addr) const
        {
            return ( (addr.pos & 0xf) 
                    | ((addr.layer & 0xff) << 8)
                   );
        }
    };
}

////////////////////////////////////////////////////////////////////////////////
// forward declaration

class ColorSpectrum;
class QLabel;
class QGraphicsProxyWidget;
class QGraphicsTextItem;

////////////////////////////////////////////////////////////////////////////////
// main data struct

class Detector2DView : public QWidget
{
public:
    Detector2DView(QWidget* parent = nullptr);

    void AddDetector(Detector2DItem *detector);
    void InitView();

    void ReDistributePaintingArea();
    void FillEvent(std::pair<std::vector<int>, std::vector<int>>[][MaxChamberPerLayer]);

protected:
    // the parameter for this function must be QResizeEvent
    // it cannot be QEvent, otherwise it won't take effect
    void resizeEvent(QResizeEvent *event);

private:
    QGraphicsScene *scene;
    QGraphicsView *view; 
    // detectors
    std::unordered_map<Detector2DAddress, Detector2DItem*> det;

    // for color bar
    ColorSpectrum *color_spectrum = nullptr;
    QLabel *color_label = nullptr;
    QGraphicsProxyWidget *color_bar_proxy_widget = nullptr;
    QGraphicsTextItem *low_adc_text=nullptr, *high_adc_text=nullptr;
};

#endif
