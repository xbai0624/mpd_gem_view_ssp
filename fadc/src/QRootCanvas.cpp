#include "QRootCanvas.h"

#include <TVirtualX.h>
#include <TStyle.h>

#include <QMouseEvent>

////////////////////////////////////////////////////////////////
// ctor

QRootCanvas::QRootCanvas(QWidget * parent) : QWidget(parent), fCanvas(0)
{
    setAttribute(Qt::WA_PaintOnScreen, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setUpdatesEnabled(kFALSE);
    setMouseTracking(kTRUE);
    setMinimumSize(100, 100);

    // register the QWidget in TVirtualX, giving its native window id
    int wid = gVirtualX->AddWindow((ULong_t)winId(), width(), height());
    // create the embeded ROOT TCanvas, 
    fCanvas = new TCanvas("Root Canvas", parent->width(), parent->height(), wid);
}


////////////////////////////////////////////////////////////////
// handle mouse move event

void QRootCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if(!fCanvas) return;

    if(e->buttons() & Qt::LeftButton) {
        fCanvas->HandleInput(kButton1Motion, e->x(), e->y());
    }
    else if(e->buttons() & Qt::MiddleButton){
        fCanvas->HandleInput(kButton2Motion, e->x(), e->y());
    }
    else if(e->buttons() & Qt::RightButton){
        fCanvas->HandleInput(kButton3Motion, e->x(), e->y());
    }
    else {
        fCanvas->HandleInput(kMouseMotion, e->x(), e->y());
    }
}

////////////////////////////////////////////////////////////////
// handle mouse button press event

void QRootCanvas::mousePressEvent(QMouseEvent *e)
{
    if(!fCanvas) return;

    switch(e->button()) {
        case Qt::LeftButton:
            fCanvas->HandleInput(kButton1Down, e->x(), e->y());
            emit ItemSelected();
            break;
        case Qt::MiddleButton:
            fCanvas->HandleInput(kButton2Down, e->x(), e->y());
            break;
        case Qt::RightButton:
            // doesn't work properly on Linux
            // ... adding setAttribute(Qt::WA_PaintOnScreen, true)
            // seems to cure the problem, but will cause more problems
            fCanvas->HandleInput(kButton3Down, e->x(), e->y());
            emit ItemDeSelected();
            break;
        default:
            break;
    }
}


////////////////////////////////////////////////////////////////
// handle mouse button release event

void QRootCanvas::mouseReleaseEvent(QMouseEvent *e)
{
    if(!fCanvas) return;

    switch(e->button()) {
        case Qt::LeftButton:
            fCanvas->HandleInput(kButton1Up, e->x(), e->y());
            break;
        case Qt::MiddleButton:
            fCanvas->HandleInput(kButton2Up, e->x(), e->y());
            break;
        case Qt::RightButton:
            // doesn't work properly on Linux
            // ... adding setAttribute(Qt::WA_PaintOnScreen, true)
            // seems to cure the problem, but will cause more problems
            fCanvas->HandleInput(kButton3Up, e->x(), e->y());
            break;
        default:
            break;
    }
}


////////////////////////////////////////////////////////////////
// handle resizes event

void QRootCanvas::resizeEvent(QResizeEvent *e)
{
    if(!fCanvas) return;

    fCanvas->SetCanvasSize(e->size().width(), e->size().height());
    fCanvas->Resize();
    fCanvas->Update();
}

////////////////////////////////////////////////////////////////
// handle paint event

void QRootCanvas::paintEvent([[maybe_unused]] QPaintEvent *e)
{
    if(!fCanvas) return;

    fCanvas->Resize();
    fCanvas->Update();
}

////////////////////////////////////////////////////////////////
// refresh

void QRootCanvas::Refresh()
{
    if(!fCanvas) return;

    fCanvas->Resize();
    fCanvas->Update();
}
