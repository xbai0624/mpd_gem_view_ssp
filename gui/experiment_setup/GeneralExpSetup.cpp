#include "GeneralExpSetup.h"
#include "APVStripMapping.h"
#include "GEMStruct.h"   // APV_STRIP_SIZE (128)

#include <QVBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QFont>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

namespace {

// Color palette cycled by panel index (NOT layer_id, since layer ids may
// be non-consecutive). Distinct, demo-friendly colors that read well on
// the light grey scene background.
const QColor kHitPalette[] = {
    Qt::red,
    Qt::blue,
    Qt::darkGreen,
    Qt::magenta,
    QColor(255, 128, 0),   // orange
    Qt::cyan,
    Qt::darkYellow,
    Qt::darkCyan,
};
constexpr int kHitPaletteSize =
    sizeof(kHitPalette) / sizeof(kHitPalette[0]);

} // namespace

// ---------------------------------------------------------------------------
// Per-layer panel: one QGraphicsScene/View pair plus the static frame and
// APV-box items. Scene units are millimeters (matches PRadSetup) so the
// schematic is to scale; fitInView handles the on-screen scaling.

struct GeneralExpSetup::LayerPanel
{
    int layer_id = 0;

    double origin_x = 0.0;
    double origin_y = 0.0;

    double det_w_mm = 0.0;
    double det_h_mm = 0.0;

    double x_offset = 0.0;
    double y_offset = 0.0;
    int    x_flip   = 1;
    int    y_flip   = 1;

    QColor hit_color = Qt::black;

    // hit dots for the current event (cleared on every PassData call)
    std::vector<QGraphicsItem*> hit_items;
};

// ---------------------------------------------------------------------------

GeneralExpSetup::GeneralExpSetup(QWidget *parent) : QWidget(parent)
{
    BuildGrid();
}

GeneralExpSetup::~GeneralExpSetup()
{
    for(auto *p : panels) delete p;
}

void GeneralExpSetup::BuildGrid()
{
    auto *mapping = apv_strip_mapping::Mapping::Instance();
    const auto &layer_map = mapping->GetLayerMap();

    std::vector<int> layer_ids;
    layer_ids.reserve(layer_map.size());
    for(const auto &kv : layer_map)
        layer_ids.push_back(kv.first);
    std::sort(layer_ids.begin(), layer_ids.end());

    const int N = static_cast<int>(layer_ids.size());
    if(N == 0)
        return;

    const int cols = static_cast<int>(std::ceil(std::sqrt(double(N))));

    scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(QColor("#ECECEC"));

    view = new QGraphicsView(this);
    view->setFrameShape(QFrame::NoFrame);
    view->setRenderHint(QPainter::Antialiasing);
    view->setScene(scene);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);
    v->addWidget(view);
    setLayout(v);

    const double box_thickness = 110.0;
    const double box_gap       = 35.0;
    const int    font_pt       = 44;
    const QFont  label_font("Times New Roman", font_pt);
    const double margin        = box_thickness + box_gap + font_pt + 20;
    const double cell_gap      = 180.0;

    double max_x = 0.0;
    double max_y = 0.0;

    for(int idx = 0; idx < N; ++idx)
    {
        const int layer_id = layer_ids[idx];
        const auto &info = layer_map.at(layer_id);

        auto *panel = new LayerPanel();
        panel->layer_id = layer_id;
        panel->det_w_mm = double(info.chambers_per_layer) * info.nb_apvs_x * info.x_pitch * APV_STRIP_SIZE;
        panel->det_h_mm = double(info.nb_apvs_y) * info.y_pitch * APV_STRIP_SIZE;
        panel->x_offset = info.x_offset;
        panel->y_offset = info.y_offset;
        panel->x_flip = info.x_flip;
        panel->y_flip = info.y_flip;
        panel->hit_color = kHitPalette[idx % kHitPaletteSize];

        const int row = idx / cols;
        const int col = idx % cols;
        panel->origin_x = col * (panel->det_w_mm + 2 * margin + cell_gap) + margin;
        panel->origin_y = row * (panel->det_h_mm + 2 * margin + cell_gap) + margin;

        // ---- detector active-area frame ---------------------------------
        auto *frame = new QGraphicsRectItem(
            panel->origin_x, panel->origin_y,
            panel->det_w_mm, panel->det_h_mm);
        frame->setPen(QPen(Qt::black, 8));
        scene->addItem(frame);

        // ---- APV boxes around the frame ---------------------------------
        // X-plane APVs along the top edge, Y-plane APVs along the left
        // edge. Box widths match APV pitch so the schematic is to scale.
        const double apv_w_mm = info.x_pitch * APV_STRIP_SIZE;
        const double apv_h_mm = info.y_pitch * APV_STRIP_SIZE;

        for(int i = 0; i < info.nb_apvs_x; ++i) {
            const double x = panel->origin_x + i * apv_w_mm;
            const double y = panel->origin_y - box_thickness - box_gap;

            auto *box = new QGraphicsRectItem(x, y, apv_w_mm, box_thickness);
            box->setBrush(QColor("#FFD0D0"));
            box->setPen(QPen(Qt::black, 4));
            scene->addItem(box);

            auto *lbl = new QGraphicsTextItem(QString::number(i));
            lbl->setFont(label_font);
            lbl->setPos(x + apv_w_mm/2 - font_pt/2,
                        y - font_pt - 4);
            scene->addItem(lbl);
        }

        for(int j = 0; j < info.nb_apvs_y; ++j) {
            const double x = panel->origin_x - box_thickness - box_gap;
            const double y = panel->origin_y + j * apv_h_mm;

            auto *box = new QGraphicsRectItem(x, y, box_thickness, apv_h_mm);
            box->setBrush(QColor("#D0E0FF"));
            box->setPen(QPen(Qt::black, 4));
            scene->addItem(box);

            auto *lbl = new QGraphicsTextItem(QString::number(j));
            lbl->setFont(label_font);
            lbl->setPos(x - font_pt - 8,
                        y + apv_h_mm/2 - font_pt/2);
            scene->addItem(lbl);
        }

        // ---- layer-id title below the frame -----------------------------
        auto *title = new QGraphicsTextItem(QString("Layer %1").arg(layer_id));
        QFont tfont("Times New Roman", 80);
        tfont.setBold(true);
        title->setFont(tfont);
        title->setDefaultTextColor(panel->hit_color);
        title->setPos(panel->origin_x + panel->det_w_mm/2 - title->boundingRect().width()/2,
                      panel->origin_y + panel->det_h_mm + box_gap);
        scene->addItem(title);

        max_x = std::max(max_x, panel->origin_x + panel->det_w_mm + margin);
        max_y = std::max(max_y, panel->origin_y + panel->det_h_mm + margin + 44 + 20);

        panels.push_back(panel);
        by_layer_id[layer_id] = panel;
    }

    scene->setSceneRect(0, 0, max_x, max_y);

    // detector_id -> layer_id table, built once from the APV map. Reused
    // by PassData() to dispatch incoming hits to the right panel.
    for(const auto &kv : mapping->GetAPVMap()) {
        const auto &api = kv.second;
        if(api.detector_id >= 0)
            det_to_layer[api.detector_id] = api.layer_id;
    }
}

// ---------------------------------------------------------------------------

void GeneralExpSetup::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if(view && scene)
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void GeneralExpSetup::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if(view && scene)
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

QSize GeneralExpSetup::sizeHint() const
{
    return QSize(400, 400);
}

void GeneralExpSetup::DrawEventHits2D(const QMap<int, QVector<QPointF>> &data)
{
    PassData(data);
}

void GeneralExpSetup::PassData(const QMap<int, QVector<QPointF>> &data)
{
    // 1) clear previous-event hits on every panel
    for(auto *p : panels) {
        for(auto *item : p->hit_items) {
            scene->removeItem(item);
            delete item;
        }
        p->hit_items.clear();
    }

    // 2) dispatch new hits (keyed by detector_id) to the matching layer
    const double dot_mm = 70.0;
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        const int det_id = it.key();
        auto lit = det_to_layer.find(det_id);
        if(lit == det_to_layer.end()) continue;

        auto pit = by_layer_id.find(lit->second);
        if(pit == by_layer_id.end()) continue;
        LayerPanel *panel = pit->second;

        for(const QPointF &hit : it.value())
        {
            // Hits arrive in detector-local mm with origin at the centre;
            // translate so (0,0) is the top-left of the frame for drawing.
            const double sx = panel->origin_x + panel->det_w_mm / 2
                            + panel->x_flip * (hit.x() - panel->x_offset);
            const double sy = panel->origin_y + panel->det_h_mm / 2
                            - panel->y_flip * (hit.y() - panel->y_offset);

            auto *dot = new QGraphicsEllipseItem(
                sx - dot_mm/2, sy - dot_mm/2, dot_mm, dot_mm);
            dot->setBrush(panel->hit_color);
            dot->setPen(QPen(panel->hit_color, 14));
            scene->addItem(dot);
            panel->hit_items.push_back(dot);
        }
    }
}
