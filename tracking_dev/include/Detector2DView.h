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
#include <map>

class QLabel;
class QGraphicsProxyWidget;
class QGraphicsTextItem;

namespace tracking_dev {

////////////////////////////////////////////////////////////////////////////////
// main data struct

class Detector2DView : public QWidget
{
public:
    Detector2DView(QWidget* parent = nullptr);

    void AddDetector(Detector2DItem *detector);
    void InitView();

    void ReDistributePaintingArea();
    void Refresh();

    void BringUpPreviousEvent(int);
    
protected:
    // the parameter for this function must be QResizeEvent
    // it cannot be QEvent, otherwise it won't take effect
    void resizeEvent(QResizeEvent *event);

private:
    QGraphicsScene *scene;
    QGraphicsView *view; 

    // detectors
    std::map<size_t, Detector2DItem*> det;
};

};

#endif
