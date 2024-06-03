#ifndef PIXEL_PADS_H
#define PIXEL_PADS_H

#include <QGraphicsItem>
#include <QColor>
#include <vector>
#include <unordered_map>
#include <map>

class ColorBar;

namespace pixel {
    // the coordinate (ix, iy) should be like the following:
    // -----------> ix
    // |
    // |
    // |
    // v iy
    // where top-left corner is the origin (0, 0)
    struct geometry_t {
        int ix, iy;
        double x_pos, y_pos;
        double width, height;
        QColor color;

        geometry_t() :
            ix(0), iy(0), x_pos(0), y_pos(0), width(0), height(0), color(Qt::gray)
        {}

        geometry_t(int nx, int ny, double x, double y, double dx, double dy):
            ix(nx), iy(ny), x_pos(x), y_pos(y), width(dx), height(dy), color(Qt::gray)
        {}

        bool operator==(const geometry_t &k ) const {
            return (k.ix == ix ) && (k.iy == iy);
        }
    };
}

template<> struct std::hash<pixel::geometry_t> {
    std::size_t operator()(const pixel::geometry_t & k) const {
        return (size_t)((k.iy & 0xffff << 16) | (k.ix & 0xffff) );
    }
};

namespace pixel {
    class PixelPads : public QGraphicsItem{
    public:
        PixelPads();
        ~PixelPads();

        QRectF boundingRect() const;
        void paint(QPainter *painter,
                const QStyleOptionGraphicsItem *option = nullptr, QWidget *widget = nullptr);
        virtual void resizeEvent();
        void SetBoundingRect(const QRectF &f);

        void SetupPads();
        void DrawPads(QPainter *painter);

    private:
        QRectF _boundingRect;

        std::map<int, geometry_t> pads;

        std::string readout_type = "PIXEL";
        // drawing range
        float area_x1, area_x2, area_y1, area_y2;

        // convert adc to rgb color
        ColorBar *color_bar;
    };
}

#endif
