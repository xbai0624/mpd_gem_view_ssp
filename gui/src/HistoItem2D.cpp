#include "HistoItem2D.h"

#include <algorithm>
#include <cmath>

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStringList>

HistoItem2D::HistoItem2D()
{
    updateDrawingRange();
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

void HistoItem2D::SetBoundingRect(const QRectF &f)
{
    prepareGeometryChange();
    bounding_rect = mapRectFromScene(f);
    bounding_rect.setRect(0, 0, bounding_rect.width(), bounding_rect.height());
    setPos(f.x(), f.y());
    updateDrawingRange();
    m_cacheDirty = true;
}

void HistoItem2D::SetTitle(const std::string &t)
{
    m_title = QString::fromStdString(t);
    update();
}

void HistoItem2D::SetStats(const std::vector<std::string> &stats)
{
    m_stats.clear();
    for(const std::string &line : stats)
        m_stats << QString::fromStdString(line);
    update();
}

void HistoItem2D::SetData(int nx, double xMin, double xMax,
                          int ny, double yMin, double yMax,
                          const std::vector<double> &z)
{
    m_nx = std::max(0, nx);
    m_ny = std::max(0, ny);
    m_xMin = xMin;
    m_xMax = xMax > xMin ? xMax : xMin + 1.0;
    m_yMin = yMin;
    m_yMax = yMax > yMin ? yMax : yMin + 1.0;
    m_viewXMin = m_xMin;
    m_viewXMax = m_xMax;
    m_viewYMin = m_yMin;
    m_viewYMax = m_yMax;

    m_z = QVector<double>(m_nx * m_ny, 0.0);
    const int nCopy = std::min<int>(m_z.size(), static_cast<int>(z.size()));
    for(int i=0; i<nCopy; ++i)
        m_z[i] = z[i];

    m_zMin = 0.0;
    m_zMax = 0.0;
    if(!m_z.isEmpty()) {
        m_zMin = m_zMax = m_z[0];
        for(double v : m_z) {
            if(v < m_zMin) m_zMin = v;
            if(v > m_zMax) m_zMax = v;
        }
    }

    m_cacheDirty = true;
    update();
}

QRectF HistoItem2D::boundingRect() const
{
    return bounding_rect;
}

void HistoItem2D::updateDrawingRange()
{
    const double left = bounding_rect.width() * 0.13;
    const double top = bounding_rect.height() * 0.12;
    const double right = bounding_rect.width() * 0.18;
    const double bottom = bounding_rect.height() * 0.16;

    drawing_range = QRectF(left, top,
                           std::max(1.0, bounding_rect.width() - left - right),
                           std::max(1.0, bounding_rect.height() - top - bottom));
}

void HistoItem2D::paint(QPainter *painter,
        [[maybe_unused]] const QStyleOptionGraphicsItem *option,
        [[maybe_unused]] QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    if(m_cacheDirty || m_cachedSize != drawing_range.size().toSize())
        rebuildHeatmapCache();

    painter->fillRect(bounding_rect, Qt::white);
    if(!m_cachedImage.isNull())
        painter->drawImage(drawing_range, m_cachedImage);

    painter->setRenderHint(QPainter::Antialiasing, true);
    drawAxes(painter);
    drawColorbar(painter);
    drawTitle(painter);
    drawStatsBox(painter);
    drawSelectBox(painter);
}

void HistoItem2D::rebuildHeatmapCache()
{
    updateDrawingRange();
    const QSize size(std::max(1, static_cast<int>(drawing_range.width())),
                     std::max(1, static_cast<int>(drawing_range.height())));
    m_cachedSize = size;
    m_cachedImage = QImage(size, QImage::Format_RGB32);
    m_cachedImage.fill(Qt::white);

    if(m_nx <= 0 || m_ny <= 0 || m_z.isEmpty()) {
        m_cacheDirty = false;
        return;
    }

    double zMin = 0.0, zMax = 0.0;
    bool haveValue = false;
    for(int iy=0; iy<m_ny; ++iy) {
        const double y = m_yMin + (iy + 0.5) / m_ny * (m_yMax - m_yMin);
        if(y < m_viewYMin || y > m_viewYMax) continue;
        for(int ix=0; ix<m_nx; ++ix) {
            const double x = m_xMin + (ix + 0.5) / m_nx * (m_xMax - m_xMin);
            if(x < m_viewXMin || x > m_viewXMax) continue;
            const double v = m_z[iy * m_nx + ix];
            if(!haveValue) {
                zMin = zMax = v;
                haveValue = true;
            } else {
                if(v < zMin) zMin = v;
                if(v > zMax) zMax = v;
            }
        }
    }
    if(haveValue) {
        m_zMin = zMin;
        m_zMax = zMax;
    }

    const double xSpan = m_viewXMax - m_viewXMin;
    const double ySpan = m_viewYMax - m_viewYMin;
    const double zSpan = m_zMax - m_zMin;

    for(int py=0; py<size.height(); ++py) {
        const double y = m_viewYMax - (py + 0.5) / size.height() * ySpan;
        int iy = static_cast<int>(std::floor((y - m_yMin) / (m_yMax - m_yMin) * m_ny));
        iy = std::clamp(iy, 0, m_ny - 1);
        QRgb *line = reinterpret_cast<QRgb*>(m_cachedImage.scanLine(py));
        for(int px=0; px<size.width(); ++px) {
            const double x = m_viewXMin + (px + 0.5) / size.width() * xSpan;
            int ix = static_cast<int>(std::floor((x - m_xMin) / (m_xMax - m_xMin) * m_nx));
            ix = std::clamp(ix, 0, m_nx - 1);
            const double z = m_z[iy * m_nx + ix];
            if(z == 0.0) {
                line[px] = qRgb(255, 255, 255);
                continue;
            }
            const double norm = zSpan > 0.0 ? (z - m_zMin) / zSpan : 0.5;
            line[px] = paletteColor(norm);
        }
    }

    m_cacheDirty = false;
}

QRgb HistoItem2D::paletteColor(double zNorm) const
{
    const double t = std::clamp(zNorm, 0.0, 1.0);
    const double r = std::clamp(1.5 * t - 0.25, 0.0, 1.0);
    const double g = std::clamp(1.5 - std::abs(3.0 * t - 1.5), 0.0, 1.0);
    const double b = std::clamp(1.25 - 1.5 * t, 0.0, 1.0);
    return qRgb(static_cast<int>(255*r),
                static_cast<int>(255*g),
                static_cast<int>(255*b));
}

void HistoItem2D::drawAxes(QPainter *painter)
{
    QPen pen(Qt::black);
    painter->setPen(pen);
    painter->drawRect(drawing_range);

    QFont font;
    const int fontSize = std::max(7, static_cast<int>(bounding_rect.width() / 55));
    font.setPixelSize(fontSize);
    painter->setFont(font);
    QFontMetrics fm(font);
    const double tickLength = std::max(3.0, bounding_rect.width() / 120.0);

    const QVector<double> xTicks = getAxisTicks(m_viewXMin, m_viewXMax);
    for(double tick : xTicks) {
        const double x = drawing_range.left()
            + (tick - m_viewXMin) / (m_viewXMax - m_viewXMin) * drawing_range.width();
        painter->drawLine(QPointF(x, drawing_range.bottom()),
                          QPointF(x, drawing_range.bottom() + tickLength));
        const QString label = QString::number(tick, 'g', 4);
        painter->drawText(QPointF(x - fm.horizontalAdvance(label) / 2.0,
                                  drawing_range.bottom() + tickLength + fm.height()),
                          label);
    }

    const QVector<double> yTicks = getAxisTicks(m_viewYMin, m_viewYMax);
    for(double tick : yTicks) {
        const double y = drawing_range.bottom()
            - (tick - m_viewYMin) / (m_viewYMax - m_viewYMin) * drawing_range.height();
        painter->drawLine(QPointF(drawing_range.left() - tickLength, y),
                          QPointF(drawing_range.left(), y));
        const QString label = QString::number(tick, 'g', 4);
        painter->drawText(QPointF(drawing_range.left() - tickLength
                                  - fm.horizontalAdvance(label) - 2,
                                  y + fm.height() / 3.0),
                          label);
    }
}

void HistoItem2D::drawColorbar(QPainter *painter)
{
    const double barW = std::max(8.0, bounding_rect.width() * 0.025);
    const QRectF bar(drawing_range.right() + bounding_rect.width() * 0.035,
                     drawing_range.top(), barW, drawing_range.height());
    for(int y=0; y<std::max(1, static_cast<int>(bar.height())); ++y) {
        const double norm = 1.0 - y / std::max(1.0, bar.height() - 1.0);
        painter->setPen(QColor::fromRgb(paletteColor(norm)));
        painter->drawLine(QPointF(bar.left(), bar.top() + y),
                          QPointF(bar.right(), bar.top() + y));
    }

    painter->setPen(Qt::black);
    painter->drawRect(bar);

    QFontMetrics fm(painter->font());
    const QString zMax = QString::number(m_zMax, 'g', 4);
    const QString zMin = QString::number(m_zMin, 'g', 4);
    painter->drawText(QPointF(bar.right() + 4, bar.top() + fm.height() / 2.0), zMax);
    painter->drawText(QPointF(bar.right() + 4, bar.bottom()), zMin);
}

void HistoItem2D::drawTitle(QPainter *painter)
{
    QFont font = painter->font();
    font.setBold(true);
    font.setPixelSize(std::max(8, static_cast<int>(bounding_rect.width() / 45)));
    painter->setFont(font);
    QFontMetrics fm(font);
    painter->setPen(Qt::black);
    painter->drawText(QPointF(drawing_range.center().x() - fm.horizontalAdvance(m_title) / 2.0,
                              drawing_range.top() - fm.height() * 0.35),
                      m_title);
}

void HistoItem2D::drawStatsBox(QPainter *painter)
{
    if(m_stats.isEmpty())
        return;

    QFont font;
    font.setPixelSize(std::max(7, static_cast<int>(bounding_rect.width() / 65)));
    painter->setFont(font);
    QFontMetrics fm(font);

    int textWidth = 0;
    for(const QString &line : m_stats)
        textWidth = std::max(textWidth, fm.horizontalAdvance(line));

    const double pad = std::max(3.0, bounding_rect.width() / 180.0);
    const double boxW = textWidth + 2.0 * pad;
    const double boxH = m_stats.size() * fm.height() + 2.0 * pad;
    QRectF box(drawing_range.right() - boxW - pad,
               drawing_range.top() + pad,
               boxW, boxH);

    painter->setPen(QPen(Qt::black));
    painter->setBrush(QColor(255, 255, 255, 220));
    painter->drawRect(box);

    painter->setPen(Qt::black);
    double y = box.top() + pad + fm.ascent();
    for(const QString &line : m_stats) {
        painter->drawText(QPointF(box.left() + pad, y), line);
        y += fm.height();
    }
}

void HistoItem2D::drawSelectBox(QPainter *painter)
{
    if(!m_selecting)
        return;
    QRectF box(m_selStart, m_selEnd);
    box = box.normalized().intersected(drawing_range);
    if(box.width() <= 1 || box.height() <= 1)
        return;

    painter->setPen(QPen(QColor(30, 120, 210), 1, Qt::DashLine));
    painter->setBrush(QColor(30, 120, 210, 35));
    painter->drawRect(box);
}

void HistoItem2D::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    const QPointF p = mapFromScene(event->scenePos());
    if(event->button() == Qt::LeftButton && drawing_range.contains(p)) {
        m_selecting = true;
        m_selStart = p;
        m_selEnd = p;
        update();
    } else if(event->button() == Qt::RightButton) {
        m_viewXMin = m_xMin;
        m_viewXMax = m_xMax;
        m_viewYMin = m_yMin;
        m_viewYMax = m_yMax;
        m_cacheDirty = true;
        update();
    }
}

void HistoItem2D::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(!m_selecting)
        return;
    m_selEnd = mapFromScene(event->scenePos());
    update();
}

void HistoItem2D::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() != Qt::LeftButton || !m_selecting)
        return;

    QRectF box(m_selStart, mapFromScene(event->scenePos()));
    box = box.normalized().intersected(drawing_range);
    m_selecting = false;

    if(box.width() > 5 && box.height() > 5) {
        const double x1 = m_viewXMin
            + (box.left() - drawing_range.left()) / drawing_range.width()
            * (m_viewXMax - m_viewXMin);
        const double x2 = m_viewXMin
            + (box.right() - drawing_range.left()) / drawing_range.width()
            * (m_viewXMax - m_viewXMin);
        const double y1 = m_viewYMax
            - (box.bottom() - drawing_range.top()) / drawing_range.height()
            * (m_viewYMax - m_viewYMin);
        const double y2 = m_viewYMax
            - (box.top() - drawing_range.top()) / drawing_range.height()
            * (m_viewYMax - m_viewYMin);

        m_viewXMin = x1;
        m_viewXMax = x2;
        m_viewYMin = y1;
        m_viewYMax = y2;
        m_cacheDirty = true;
    }
    update();
}

QVector<double> HistoItem2D::getAxisTicks(double a_min, double a_max) const
{
    QVector<double> ticks;
    if(a_max <= a_min)
        return ticks;

    const double span = a_max - a_min;
    const double rawStep = span / 4.0;
    const double mag = std::pow(10.0, std::floor(std::log10(rawStep)));
    const double norm = rawStep / mag;
    double step = mag;
    if(norm > 5.0) step = 10.0 * mag;
    else if(norm > 2.0) step = 5.0 * mag;
    else if(norm > 1.0) step = 2.0 * mag;

    const double first = std::ceil(a_min / step) * step;
    for(double v = first; v <= a_max + step * 0.25; v += step)
        ticks.push_back(v);
    return ticks;
}
