#ifndef COMPONENTSSCHEMATIC_H
#define COMPONENTSSCHEMATIC_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include <vector>

class ComponentsSchematic : public QWidget
{
    Q_OBJECT

public:
    ComponentsSchematic(QWidget *parent = 0);
    ~ComponentsSchematic();

    void Init();

public slots:
    void ItemSelected();
    void ItemDeSelected();

protected:
    virtual void paintEvent(QPaintEvent *e);

protected:
    QGraphicsView *graphics_view;
    QGraphicsScene *graphics_scene;

    std::vector<QGraphicsItem*> item_list;
};

#endif
