#include "PixelPads.h"
#include "PixelMapping.h"
#include "ColorBar.h"
#include <QHBoxLayout>
#include <QPointF>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace pixel {
    // a temporary solution
    static std::unordered_map<std::string, std::pair<int, int>> apv_name_iloc = {
        {"APV-A", std::pair<int, int>(3, 13)},
        {"APV-B", std::pair<int, int>(3, 5)},
        {"APV-C", std::pair<int, int>(12, 17)},
        {"APV-D", std::pair<int, int>(12, 5)},
        {"APV-E", std::pair<int, int>(23, 2)},
        {"APV-F", std::pair<int, int>(23, 13)},
        {"APV-G", std::pair<int, int>(23, 26)},
        {"APV-H", std::pair<int, int>(23, 40)},
    };
    static std::unordered_map<std::string, std::pair<float, float>> apv_name_floc = {
        {"APV-A", std::pair<float, float>(0, 0)},
        {"APV-B", std::pair<float, float>(0, 0)},
        {"APV-C", std::pair<float, float>(0, 0)},
        {"APV-D", std::pair<float, float>(0, 0)},
        {"APV-E", std::pair<float, float>(0, 0)},
        {"APV-F", std::pair<float, float>(0, 0)},
        {"APV-G", std::pair<float, float>(0, 0)},
        {"APV-H", std::pair<float, float>(0, 0)},
    };
    static std::unordered_map<std::string, std::string> apv_name_sloc = {
        {"APV-A", "adc_ch6"},
        {"APV-B", "adc_ch1"},
        {"APV-C", "adc_ch7"},
        {"APV-D", "adc_ch0"},
        {"APV-E", "adc_ch3"},
        {"APV-F", "adc_ch2"},
        {"APV-G", "adc_ch5"},
        {"APV-H", "adc_ch4"},
    };

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
        QColor _color[8] = { QColor(0, 0, 205), QColor(0, 0, 0), QColor(148, 0, 211),
            QColor(139, 69, 19), QColor(205, 133, 63), QColor(119, 136, 153),
            QColor(0, 255, 0), QColor(128, 0, 128)};
        std::unordered_map<std::string, QColor> name_to_color = {
            {"APV-A", _color[0]},
            {"APV-B", _color[1]},
            {"APV-C", _color[2]},
            {"APV-D", _color[3]},
            {"APV-E", _color[4]},
            {"APV-F", _color[5]},
            {"APV-G", _color[6]},
            {"APV-H", _color[7]},
        };

        double _w = _boundingRect.width();
        double _h = _boundingRect.height();

        double length = _w > _h ? _h : _w;

        int pad_counter = 0;

        auto construct_section = [&](int row_start, int row_end, int nCols, double row_pos_off, double col_pos_off) {
            // each row consists of the same type pads
            double width = length / (double)nCols;

            for(int i=row_start; i<row_end; i++) {
                for(int j=0; j<nCols; j++) {
                    std::string apv_name = pixel::PixelMapping::Instance().GetAPVNameFromCoord(i, j);
                    QColor c = name_to_color[apv_name];
                    double row_pos = row_pos_off + (i-row_start) * width;
                    double col_pos = col_pos_off + j * width;
                    ro_pads[pad_counter] = new geometry_t(i, j, col_pos, row_pos, width, width);
                    ro_pads[pad_counter] -> color = c;
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
                    std::string name = pixel::PixelMapping::Instance().GetAPVNameFromCoord(i, j);

                    double row_pos = row_pos_off + (i-row_start) * width;
                    double col_pos = col_pos_off + j * width;
                    ro_pads[pad_counter] -> x_pos = col_pos;
                    ro_pads[pad_counter] -> y_pos = row_pos;
                    pad_counter++;

                    if(i == apv_name_iloc[name].first && j == apv_name_iloc[name].second)
                        apv_name_floc[name].first = col_pos, apv_name_floc[name].second = row_pos;
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

            //i.second -> color = Qt::gray;
            if(i.second -> event_adc > 1) {
                //i.second -> color = color_bar->toColor(i.second -> event_adc);
                QColor color = color_bar->toColor(i.second -> event_adc);
                painter -> setPen(color);
                painter -> setBrush(color);
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

        // draw apv channel name on the canvas
        painter -> save();
        //QFont font = painter -> font();
        //font.setPixelSize(12);
        painter -> setPen(Qt::black);
        painter -> setFont(QFont("times", 22));
        for(auto &i: apv_name_iloc) {
            painter -> drawText(apv_name_floc[i.first].first, apv_name_floc[i.first].second, QString(apv_name_sloc[i.first].c_str()));
        }
        painter -> restore();

    }

    void PixelPads::ReceiveContents(const std::vector<std::pair<int, float>> &_x_strips,
            const std::vector<std::pair<int, float>> &_y_strips)
    {
        // reset event
        for(auto &i: ro_pads) {
            //i.second -> color = Qt::gray;
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
