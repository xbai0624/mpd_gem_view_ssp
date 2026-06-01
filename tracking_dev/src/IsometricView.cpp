#include "IsometricView.h"
#include "VirtualDetector.h"
#include "Detector2DHitItem.h"   // CACHE_EVENT_SIZE
#include "tracking_struct.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QPolygonF>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

namespace tracking_dev {

namespace {
    // 30-degree isometric basis.
    constexpr double kCos30 = 0.8660254037844387;
    constexpr double kSin30 = 0.5;
}

// Classic isometric: +x and +y span the floor. +z points DOWN the page
// (Qt y-down) so z=0 sits at the top and the beam-direction reading
// order matches what physicists expect.
QPointF IsometricView::project(double x, double y, double z)
{
    return QPointF((x - y) * kCos30,
                   -(x + y) * kSin30 + z);
}

IsometricView::IsometricView(QWidget *parent)
    : QWidget(parent), scene(nullptr), view(nullptr)
{
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
}

IsometricView::~IsometricView() = default;

void IsometricView::AddLayer(VirtualDetector *det)
{
    if(det) layers.push_back(det);
}

void IsometricView::InitView()
{
    if(layers.empty()) return;

    // Sort low-z first so it ends up at the top of the screen (Qt's
    // y-axis points down and our projection adds z).
    std::sort(layers.begin(), layers.end(),
              [](VirtualDetector *a, VirtualDetector *b){
                  return a->GetZPosition() < b->GetZPosition();
              });

    // Largest detector width drives label font and hit radii so the
    // view scales sensibly for any geometry.
    max_dim = 0.0;
    for(auto *d : layers)
        max_dim = std::max(max_dim, d->GetDimension().x);

    const int    font_pt   = std::max(12,  int(max_dim * 0.06));
    const double frame_pen = std::max(2.0, max_dim * 0.005);

    // Iterate in reverse z-order so top-of-screen (low z) frames are
    // drawn last and stay fully visible (solid black); bottom-of-screen
    // (high z) frames are drawn first and the later translucent overlays
    // make their lines look lighter where they cross. Restores the
    // front/back occlusion look from before the z-flip.
    for(auto it = layers.rbegin(); it != layers.rend(); ++it) {
        auto *det = *it;
        const auto &origin = det->GetOrigin();
        const auto &dim    = det->GetDimension();
        const double cx = origin.x, cy = origin.y, cz = origin.z;
        const double hw = dim.x * 0.5, hh = dim.y * 0.5;

        // detector frame as a parallelogram (4 projected corners)
        QPolygonF poly;
        poly << project(cx - hw, cy - hh, cz)
             << project(cx + hw, cy - hh, cz)
             << project(cx + hw, cy + hh, cz)
             << project(cx - hw, cy + hh, cz);

        auto *frame = new QGraphicsPolygonItem(poly);
        frame->setPen(QPen(Qt::black, frame_pen));
        frame->setBrush(QBrush(QColor(255, 255, 255, 90)));
        scene->addItem(frame);

        // layer-id label, anchored at the back-left corner
        const QPointF lpos = project(cx - hw, cy + hh, cz);
        auto *lbl = new QGraphicsTextItem(
            QString("Layer %1, z = %2 mm")
                .arg(det->GetLayerID())
                .arg(int(cz)));
        QFont f("Times New Roman", font_pt);
        lbl->setFont(f);
        // shift the label left so it doesn't overlap the frame edge
        lbl->setPos(lpos.x() - font_pt * 8, lpos.y());
        scene->addItem(lbl);
    }

    // sceneRect = bounding box of everything we drew, with a margin
    const QRectF rc = scene->itemsBoundingRect();
    const double m  = std::max(50.0, max_dim * 0.1);
    scene->setSceneRect(rc.adjusted(-m, -m, m, m));
}

void IsometricView::Refresh()
{
    // 1) drop everything we drew for the previous event
    for(auto *it : dynamic_items) {
        scene->removeItem(it);
        delete it;
    }
    dynamic_items.clear();

    if(layers.empty()) return;

    // 2) pick the frame to draw: cached older event or live current event
    EventFrame frame;
    if(counter <= 0) {
        // step-back path: pull from cache, do NOT push (mirror of
        // Detector2DHitItem::UpdateEventContent's prev-event branch).
        int index = -counter;
        counter = 1;
        if(index < 0 || index >= (int)event_cache.size()) return;
        frame = event_cache.at(index);
    } else {
        // forward path: read live detector hits, push to cache (capped).
        frame = CaptureCurrentFrame();
        event_cache.push_front(frame);
        if((int)event_cache.size() > CACHE_EVENT_SIZE)
            event_cache.pop_back();
    }

    DrawFrame(frame);
}

IsometricView::EventFrame IsometricView::CaptureCurrentFrame()
{
    EventFrame f;
    f.bg.resize(layers.size());
    f.real.resize(layers.size());
    f.fitted.resize(layers.size());
    f.track_pts.reserve(layers.size());

    for(size_t li = 0; li < layers.size(); ++li) {
        auto *det = layers[li];

        for(const auto &p : det->GetBackgroundHits())
            f.bg[li].push_back(project(p.x, p.y, p.z));

        for(const auto &p : det->GetRealHits())
            f.real[li].push_back(project(p.x, p.y, p.z));

        const auto &fits = det->GetFittedHits();
        for(size_t i = 0; i < fits.size(); ++i) {
            const QPointF q = project(fits[i].x, fits[i].y, fits[i].z);
            f.fitted[li].push_back(q);
            if(i == 0) f.track_pts.push_back(q);
        }
    }
    return f;
}

void IsometricView::DrawFrame(const EventFrame &frame)
{
    // Hit radii / line widths scaled to the geometry so they're readable
    // at any fitInView zoom level.
    const double r_dot    = std::max(4.0, max_dim * 0.005);
    const double r_circle = std::max(8.0, max_dim * 0.010);
    const double pen_w    = std::max(1.5, max_dim * 0.0015);
    const double track_w  = std::max(2.0, max_dim * 0.0025);

    auto add_ellipse = [&](double cx, double cy, double r,
                           QColor color, bool filled)
    {
        auto *e = new QGraphicsEllipseItem(cx - r, cy - r, 2*r, 2*r);
        e->setPen(QPen(color, pen_w));
        e->setBrush(filled ? QBrush(color) : QBrush(Qt::NoBrush));
        scene->addItem(e);
        dynamic_items.push_back(e);
    };

    for(size_t li = 0; li < layers.size(); ++li) {
        // background -> small black dots
        for(const auto &q : frame.bg[li])
            add_ellipse(q.x(), q.y(), r_dot, Qt::black, true);
        // real (selected) -> blue rings
        for(const auto &q : frame.real[li])
            add_ellipse(q.x(), q.y(), r_circle, Qt::blue, false);
        // fitted -> red rings
        for(const auto &q : frame.fitted[li])
            add_ellipse(q.x(), q.y(), r_circle, Qt::red, false);
    }

    // red polyline through the fitted hits = the reconstructed track
    for(size_t i = 1; i < frame.track_pts.size(); ++i) {
        auto *seg = new QGraphicsLineItem(
            frame.track_pts[i-1].x(), frame.track_pts[i-1].y(),
            frame.track_pts[i].x(),   frame.track_pts[i].y());
        seg->setPen(QPen(Qt::red, track_w));
        scene->addItem(seg);
        dynamic_items.push_back(seg);
    }
}

void IsometricView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if(view && scene)
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void IsometricView::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if(view && scene)
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

}; // namespace tracking_dev
