#include "Detector2DView.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <iostream>

namespace tracking_dev {

Detector2DView::Detector2DView(QWidget* parent) : QWidget(parent),
    scene(new QGraphicsScene(this)), view(new QGraphicsView(scene))
{
}

void Detector2DView::InitView()
{
    //view->setStyleSheet("background:transparent");
    for(auto &l: det)
    {
        scene -> addItem(l.second);
    }

    ReDistributePaintingArea();

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout -> addWidget(view);
}

void Detector2DView::ReDistributePaintingArea()
{
    // need to minus 20 to allow space for vertical and horizontal scroll bars
    int sceneRectWidth = width() - 20 > 0 ? width() - 20 : 100;
    int sceneRectHeight = height() - 20 > 0 ? height() - 20 : 50;

    QRectF f(0, 0, sceneRectWidth, sceneRectHeight);
    scene -> setSceneRect(f); // set scene coordinate frame

    // distribute painting area

    int layer_index = 0, NLayer = (int)det.size();
    for(auto &l: det)
    {
        QRectF f(layer_index*sceneRectWidth/NLayer, 0, 
                sceneRectWidth/NLayer, sceneRectHeight);

        l.second -> setPos(f.x(), f.y()); // move to scene coord (f.x(), f.y())

        // maybe it's better not to change boundingRect
        // otherwise the ratio will be changed
        l.second -> SetBoundingRect(f);

        layer_index++;
    }
}

void Detector2DView::resizeEvent([[maybe_unused]] QResizeEvent *event)
{
    ReDistributePaintingArea();
}

void Detector2DView::AddDetector(Detector2DItem *item)
{
    size_t s = det.size();
    det[s] = item;
}

void Detector2DView::Refresh()
{
    for(auto &i: det)
        i.second -> update();
}

void Detector2DView::BringUpPreviousEvent(int prev_event_number)
{
    for(auto &i: det)
        i.second -> SetCounter(prev_event_number);
}

};
