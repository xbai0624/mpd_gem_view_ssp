/*
 * PRad GEM Detector Size:
 * GEMX, 550.4 mm, 12 APVs, with one single slot covering beam hole
 * GEMY, 1228.8mm, 24 APVs
 * each APV covers 51.2mm, the X plane has shorter effective distance
 * because in combined 32 strips (on 2 APVs, 16 strips on each APV) are
 * not connected
 */

#ifndef PRAD_SETUP_H
#define PRAD_SETUP_H

#include <QWidget>
#include <vector>
#include <unordered_map>

class GEMDetector;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsItem;
class QResizeEvent;
class QColor;

class PRadSetup : public QWidget
{
    Q_OBJECT
public:
    PRadSetup(QWidget* parent, int layer_id);
    ~PRadSetup();

    void InitSetup();
    std::vector<QGraphicsItem*> drawDetectionArea();
    std::vector<QGraphicsItem*> drawSpacers();
    std::vector<QGraphicsItem*> drawAPVs();
    std::vector<QGraphicsItem*> drawArrow(double x_pos, double y_pos, double angle, double length, QString label, QColor c);

    void PassData(const QMap<int, QVector<QPointF>> &data);

    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    QSize sizeHint() const override;

public slots:
    void DrawEventHits2D(const QMap<int, QVector<QPointF>> &);

private:
    QGraphicsScene *scene;
    QGraphicsView *view;

    int layer_id = 0;

    // event data
    std::vector<QGraphicsItem*> Hits2D;

    // detector geometry
    double det_w = 1100.4;
    double det_h = 1228.8;
    double overlap_d = 44.0;
    double hole_diameter = 44.0;
    double horizontal_spacer_pos[6];
    double vertical_spacer_pos[2];
};

#endif
