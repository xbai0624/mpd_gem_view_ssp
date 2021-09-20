/*
 * This class is to propagate mouse event from widget to QGraphicsItem
 */

#ifndef HISTO_VIEW_H
#define HISTO_VIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>

class HistoView : public QGraphicsView
{
public:
    HistoView(QGraphicsScene *scene);

protected:
    void mousePressEvent(QMouseEvent *event)
    {
        QGraphicsView::mousePressEvent(event);
    }
};

#endif
