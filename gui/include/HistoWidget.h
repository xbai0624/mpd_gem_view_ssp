#ifndef HISTO_WIDGET_H
#define HISTO_WIDGET_H

#include <vector>
#include <QGraphicsScene>
#include <QGraphicsView>
//#include "HistoView.h"
#include "HistoItem.h"
#include "HistoItem2D.h"

#include "MPDDataStruct.h"

class HistoWidget : public QWidget
{
    Q_OBJECT
public:
    struct PlotData {
        enum Type { Plot1D, Plot2D } type = Plot1D;
        std::string title;
        std::vector<std::string> stats;

        std::vector<double> y;    // 1D bin contents

        int nx = 0, ny = 0;       // 2D binning
        double xMin = 0, xMax = 1;
        double yMin = 0, yMax = 1;
        std::vector<double> z;    // 2D row-major contents
    };

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
    // draw mixed 1D/2D histograms
    void DrawCanvas(const std::vector<PlotData> &data, int, int);

    void Clear();

protected:
    void resizeEvent(QResizeEvent *e);
    void mousePressEvent(QMouseEvent *event);

private:
    struct PlotSlot {
        QGraphicsItem *item = nullptr;
        PlotData::Type type = PlotData::Plot1D;
    };

    void EnsureSlotType(int idx, PlotData::Type type);

    QGraphicsScene *scene;
    //HistoView *view;
    QGraphicsView *view;
    std::vector<PlotSlot> pItem;

    // divide the whole area into fCol by fRow sections
    int fRow = 4;
    int fCol = 4;
};

#endif
