#ifndef HISTO_WIDGET_H
#define HISTO_WIDGET_H

#include <vector>
#include <QGraphicsScene>
#include "HistoView.h"
#include "HistoItem.h"

#include "MPDDataStruct.h"
#include "QMainCanvas.h"

class HistoWidget : public QWidget
{
    Q_OBJECT
public:
    HistoWidget(QWidget *parent = nullptr);
    ~HistoWidget();

    void resize(int, int);
    void Refresh();

    void Divide(int r, int c);
    void ReDistributePaintingArea();
    void ReInitHistoItems();

    // draw histo, extract title from apv address
    void DrawCanvas(const std::vector<std::vector<int>> &data, 
            const std::vector<APVAddress> &addr, int, int);
    // draw histo, pass title directly
    void DrawCanvas(const std::vector<std::vector<int>> &data, 
            const std::vector<std::string> &title, int, int);

    void PassQMainCanvasPointer(QMainCanvas* canvas);
    void Clear();

protected:
    void resizeEvent(QResizeEvent *e);
    void mousePressEvent(QMouseEvent *event);

private:
    QGraphicsScene *scene;
    HistoView *view;
    HistoItem **pItem = nullptr;

    // divide the whole area into fCol by fRow sections
    int fRow = 4;
    int fCol = 4;
};

#endif
