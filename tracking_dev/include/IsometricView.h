#ifndef ISOMETRIC_VIEW_H
#define ISOMETRIC_VIEW_H

// Isometric 3D view of the tracking geometry.
//
// Each registered VirtualDetector is drawn as a translucent parallelogram
// (the detector's active area projected with a 30-degree isometric
// transform). Layers stack vertically because the projection adds +z
// (Qt y-down): z=0 lands at the top, larger z further down.
//
// Per event, Refresh() reads each detector's current hit lists and draws:
//   - background hits  -> small black dots   (raw hits not on the track)
//   - real hits        -> blue rings         (hits selected for the track)
//   - fitted hits      -> red rings          (track projection per layer)
//   - red line through the fitted hits       (the reconstructed track)
//
// Public API mirrors Detector2DHitView (AddLayer/InitView/Refresh) so
// wiring into the main Viewer is trivial.

#include <QWidget>
#include <QPointF>
#include <QSize>
#include <deque>
#include <vector>

class QGraphicsScene;
class QGraphicsView;
class QGraphicsItem;
class QResizeEvent;
class QShowEvent;

namespace tracking_dev {

class VirtualDetector;

class IsometricView : public QWidget
{
    Q_OBJECT
public:
    IsometricView(QWidget *parent = nullptr);
    ~IsometricView();

    void AddLayer(VirtualDetector *det);   // register one layer (no draw)
    void InitView();                       // build static frames once
    void Refresh();                        // redraw per-event dynamic items

    // Override the QGraphicsView-derived sizeHint (which equals the
    // sceneRect in mm and would otherwise inflate the right panel).
    // Keep it tiny; splitter stretch factors take over from here.
    QSize sizeHint() const override { return QSize(160, 120); }

    // Same shape as Detector2DHitView::BringUpPreviousEvent: a negative
    // counter (e.g. -3) tells the next Refresh to pull frame index 3 from
    // the cache instead of reading live detector hits.
    void BringUpPreviousEvent(int n) { counter = n; }

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent  *e) override;

private:
    QGraphicsScene *scene;
    QGraphicsView  *view;

    // Registered detectors, z-sorted by InitView.
    std::vector<VirtualDetector*> layers;

    // Items created per event; cleared at the top of every Refresh.
    std::vector<QGraphicsItem*>   dynamic_items;

    // Largest detector width in mm; cached during InitView so Refresh
    // can scale hit radii / pen widths relative to the scene.
    double max_dim = 0.0;

    // Per-event cache so Prev navigation can replay older events that
    // the underlying VirtualDetectors have long since overwritten.
    // Mirror of Detector2DHitItem's counter / *_cache machinery.
    struct EventFrame {
        // outer index = position in layers[]; inner = projected points
        std::vector<std::vector<QPointF>> bg, real, fitted;
        // one entry per layer that has a fitted hit, in z-order
        std::vector<QPointF> track_pts;
    };
    std::deque<EventFrame> event_cache;
    int counter = 1;   // 1 = forward step; <=0 = step back (-counter = index)

    static QPointF project(double x, double y, double z);

    EventFrame CaptureCurrentFrame();
    void       DrawFrame(const EventFrame &);
};

}; // namespace tracking_dev

#endif
