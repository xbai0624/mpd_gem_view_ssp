#ifndef QROOTCANVAS_H
#define QROOTCANVAS_H

#include <QWidget>
#include <TCanvas.h>
#include <TH1I.h>
#include <vector>

class QRootCanvas : public QWidget
{
    Q_OBJECT

public:
    QRootCanvas(QWidget *parent = nullptr);
    ~QRootCanvas(){};

    TCanvas *GetCanvas() {return fCanvas;};
    void Refresh();

signals:
    void ItemSelected();
    void ItemDeSelected();

protected:
    TCanvas *fCanvas;

    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual void resizeEvent(QResizeEvent *e);

};

#endif
