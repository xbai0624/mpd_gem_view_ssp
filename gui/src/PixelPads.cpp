#include "PixelPads.h"
#include "PixelMapping.h"
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

        SetupPads();
    }

    PixelPads::~PixelPads() {
    }

    QRectF PixelPads::boundingRect() const {
        return _boundingRect;
    }

    void PixelPads::SetBoundingRect(const QRectF &f)
    {
        prepareGeometryChange();

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
        RedistributePads();

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
                    //if(pixel::PixelMapping::Instance().GetAPVNameFromCoord(i, j) != "APV-G")
                    //    continue;
                    double row_pos = row_pos_off + (i-row_start) * width;
                    double col_pos = col_pos_off + j * width;
                    ro_pads[pad_counter++] = new geometry_t(i, j, col_pos, row_pos, width, width);
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

    void PixelPads::RedistributePads()
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
                    //if(pixel::PixelMapping::Instance().GetAPVNameFromCoord(i, j) != "APV-G")
                    //    continue;
 
                    double row_pos = row_pos_off + (i-row_start) * width;
                    double col_pos = col_pos_off + j * width;
                    ro_pads[pad_counter] -> x_pos = col_pos;
                    ro_pads[pad_counter] -> y_pos = row_pos;
                    pad_counter++;
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
        for(auto &i: ro_pads)
        {
            painter -> save();

            i.second -> color = Qt::gray;
            if(i.second -> event_adc > 10) {
                i.second -> color = color_bar->toColor(i.second -> event_adc);
                painter -> setPen(i.second -> color);
                painter -> setBrush(i.second -> color);
                painter -> drawRect(i.second -> x_pos, i.second -> y_pos, i.second -> width, i.second -> height);
            } else {
                painter -> setPen(i.second -> color);
                painter -> drawRect(i.second -> x_pos, i.second -> y_pos, i.second -> width, i.second -> height);
            }

            painter -> restore();

            /*
            // show mapping
            QFont font = painter->font();
            if(i.second.ix >= 18)
            font.setPointSize(6);
            else
            font.setPointSize(10);
            painter -> setFont(font);
            painter -> setPen(Qt::black);
            int ch = pixel::PixelMapping::Instance().GetStripNoFromCoord(i.second.ix, i.second.iy);
            painter -> drawText(i.second.x_pos, i.second.y_pos + i.second.height, QString::number(ch));
            */
        }
    }

    void PixelPads::ReceiveContents(const std::vector<std::pair<int, float>> &_x_strips,
            const std::vector<std::pair<int, float>> &_y_strips)
    {
        // reset event
        for(auto &i: ro_pads) {
            i.second -> color = Qt::gray;
            i.second -> event_adc = 0;
        }

        auto fill_pixel = [&](const int &row, const int &col, const float &adc)
        {
            // this is a very slow operation, temporary solution
            for(auto &i: ro_pads) {
                if(i.second -> ix == row && i.second -> iy == col){
                    i.second -> event_adc = adc;
                    i.second -> hit_accum += 1.;
                }
            }
        };

        auto fill_strip = [&](int plane, const std::pair<int, float>& i)
        {
            int strip = static_cast<int>(i.first);
            double adc = static_cast<float>(i.second);

            int pos = strip / 128;
            int ch = strip % 128; // this one must be in panasonic pin order

            std::string apv_name = pixel::PixelMapping::Instance().GetAPVNameFromPlanePos(plane, pos);
            std::pair<int, int> coord = pixel::PixelMapping::Instance().GetCoordFromStripNo(apv_name, ch);

            int row = coord.first;
            int col = coord.second;
            if(row >= 0 && col >= 0)
            {
                fill_pixel(row, col, adc);
            }
        };

        auto fill_pads = [&](const std::vector<std::pair<int, float>> &strips, bool is_x_plane)
        {
            int plane = 0;
            if(!is_x_plane) plane = 1;

            for(auto &i: strips)
            {
                fill_strip(plane, i);
            }
        };

        fill_pads(_x_strips, true);
        fill_pads(_y_strips, false);
        update();
    }
}
