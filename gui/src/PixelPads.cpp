#include "PixelPads.h"
#include "ColorBar.h"
#include <QHBoxLayout>
#include <QPointF>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace pixel {
    PixelPads::PixelPads()
    {
        _boundingRect.setRect(0, 0, 600, 600);
        color_bar = new ColorBar();
    }

    PixelPads::~PixelPads() {
    }

    QRectF PixelPads::boundingRect() const {
        return _boundingRect;
    }

    void PixelPads::SetBoundingRect(const QRectF &f)
    {
        _boundingRect = mapRectFromScene(f);
    }

    void PixelPads::resizeEvent()
    {
        QPainter painter;
        paint(&painter);
    }

    void PixelPads::paint(QPainter *painter,
            [[maybe_unused]] const QStyleOptionGraphicsItem *option,
            [[maybe_unused]] QWidget *widget)
    {
        SetupPads();
        DrawPads(painter);
    }

    void PixelPads::SetupPads()
    {
        double _w = _boundingRect.width();
        double _h = _boundingRect.height();

        double length = _w > _h ? _h : _w;

        int pad_counter = 0;

        auto construct_section = [&](int row_start, int row_end, int nCols, double row_pos_off, double col_pos_off) {
            // each row consists of the same type pads
            double width = length / (double)nCols;

            for(int i=row_start; i<row_end; i++) {
                for(int j=0; j<nCols; j++) {
                    double row_pos = row_pos_off + (i-row_start) * width;
                    double col_pos = col_pos_off + j * width;
                    pads[pad_counter++] = geometry_t(i, j, col_pos, row_pos, width, width);
                }
            }
        };

        double row_pos_off = 0, col_pos_off = 0;

        // center the drawing in both X and Y direction
        if(_w > _h) 
            col_pos_off = (_w - _h) / 2;
        else
            row_pos_off = (_h - _w) / 2;

        // sec1: 8 rows, 20 columns
        construct_section(0, 8, 20, row_pos_off, col_pos_off);
        row_pos_off += length/20 * 8.;

        // sec2: 10 rows, 25 columns
        construct_section(8, 18, 25, row_pos_off, col_pos_off);
        row_pos_off += length/25. * 10;

        // sec3: 10 rows, 50 columns
        construct_section(18, 28, 50, row_pos_off, col_pos_off);
    }

    void PixelPads::DrawPads(QPainter *painter)
    {
        for(auto &i: pads) {
            painter -> setPen(i.second.color);
            painter -> drawRect(i.second.x_pos, i.second.y_pos, i.second.width, i.second.height);
        }
    }
}
