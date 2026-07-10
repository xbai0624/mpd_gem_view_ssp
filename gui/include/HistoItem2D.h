/*******************************************************************************
 * HistoItem2D
 *
 *   Native Qt-only 2D "colz"-style heatmap, drawn as a QGraphicsItem.
 *   Sibling to HistoItem (1D), sharing the same box-zoom + right-click reset
 *   interaction model. Bin data is supplied as a flat row-major vector
 *   (z[iy*nx + ix]) plus X / Y axis ranges; the item never touches ROOT.
 *
 *   The heatmap itself is rendered into a cached QImage via per-pixel
 *   sampling; the cache is invalidated only when data, view ranges or the
 *   bounding rect change, so paint() is just a blit + axes/colorbar/title.
 ******************************************************************************/

#ifndef HISTOITEM2D_H
#define HISTOITEM2D_H

#include <QGraphicsItem>
#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

#include <string>
#include <vector>

class HistoItem2D : public QGraphicsItem {
public:
    HistoItem2D();
    ~HistoItem2D() override = default;

    void SetBoundingRect(const QRectF &f);
    void SetTitle(const std::string &t);
    void SetStats(const std::vector<std::string> &stats);
    void SetAxisTitles(const std::string &x, const std::string &y) {
        m_xAxisTitle = QString::fromStdString(x);
        m_yAxisTitle = QString::fromStdString(y);
    }
    // row-major z[iy*nx + ix]
    void SetData(int nx, double xMin, double xMax,
                 int ny, double yMin, double yMax,
                 const std::vector<double> &z);

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option = nullptr,
               QWidget *widget = nullptr) override;

protected:
    void mousePressEvent  (QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent   (QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // re-layout drawing_range from bounding_rect
    void   updateDrawingRange();
    // build m_cachedImage for current view + drawing size
    void   rebuildHeatmapCache();
    void   drawAxes(QPainter *painter);
    void   drawColorbar(QPainter *painter);
    void   drawTitle(QPainter *painter);
    void   drawAxisTitles(QPainter *painter);
    void   drawStatsBox(QPainter *painter);
    void   drawSelectBox(QPainter *painter);
    QRgb   paletteColor(double zNorm) const;
    QVector<double> getAxisTicks(double a_min, double a_max) const;

private:
    QRectF bounding_rect{0, 0, 100, 100};
    QRectF drawing_range;         // the heatmap area (excludes axes / colorbar)

    // data
    int m_nx = 0, m_ny = 0;
    double m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    double m_zMin = 0, m_zMax = 0;
    QVector<double> m_z;          // size = m_nx * m_ny, row-major

    // view ranges (subset of data ranges via box-zoom)
    double m_viewXMin = 0, m_viewXMax = 1;
    double m_viewYMin = 0, m_viewYMax = 1;

    // cached heatmap render
    QImage m_cachedImage;
    QSize  m_cachedSize;          // size used to build m_cachedImage
    bool   m_cacheDirty = true;

    // mouse zoom
    bool    m_selecting = false;
    QPointF m_selStart;
    QPointF m_selEnd;

    QString m_title = "hist";
    QString m_xAxisTitle, m_yAxisTitle;
    QStringList m_stats;
};

#endif
