#ifndef GENERAL_EXP_SETUP_H
#define GENERAL_EXP_SETUP_H

// Generic N-layer GEM experiment view.
//
// Mirrors PRadSetup's public interface (PassData + DrawEventHits2D slot
// with the same QMap<int, QVector<QPointF>> shape keyed by detector_id)
// but builds itself from APVStripMapping::Mapping at construction time:
//
//   - one mini-panel per layer found in the mapping,
//   - panels arranged in a roughly-square grid
//     (cols = ceil(sqrt(N)), rows = ceil(N/cols)),
//   - per-panel detector frame + small boxes for each APV (top edge for
//     X-plane, left edge for Y-plane), sized from LayerInfo,
//   - per-layer hit color so dots in different layers are easy to tell
//     apart in a lecture demo.
//
// Drop-in replacement for PRadSetup on the experiment-setup page: the
// existing Viewer::onlineHitsDrawn signal drives DrawEventHits2D unchanged.

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QPointF>

#include <vector>
#include <unordered_map>

class QGraphicsView;
class QGraphicsScene;
class QGraphicsItem;
class QResizeEvent;
class QShowEvent;

class GeneralExpSetup : public QWidget
{
    Q_OBJECT
public:
    explicit GeneralExpSetup(QWidget *parent = nullptr);
    ~GeneralExpSetup();

    // PRadSetup-compatible interface
    void PassData(const QMap<int, QVector<QPointF>> &data);

    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;
    QSize sizeHint() const override;

public slots:
    void DrawEventHits2D(const QMap<int, QVector<QPointF>> &);

private:
    struct LayerPanel;

    QGraphicsScene *scene = nullptr;
    QGraphicsView  *view  = nullptr;

    std::vector<LayerPanel*>            panels;
    std::unordered_map<int, LayerPanel*> by_layer_id;
    std::unordered_map<int, int>         det_to_layer;

    void BuildGrid();
};

#endif
