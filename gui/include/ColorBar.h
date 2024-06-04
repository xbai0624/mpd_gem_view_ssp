#ifndef COLOR_BAR_H
#define COLOR_BAR_H

#include <QWidget>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QLabel>

class ColorBar : public QWidget {
public: 
    ColorBar(QWidget *parent = nullptr) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setMaximumHeight(600);
    }

    QSize sizeHint() const override {
        return QSize(100, 600);
    }

    QSize minimumSizeHint() const override {
        return QSize(100, 100);
    }

    void setMaxADC(double a) {
        MAXADC = a;
    }

    QColor toColor(double adc) const {
        QLinearGradient gradient(0, 0, 0, MAXADC);

        gradient.setColorAt(0.0, Qt::blue);
        gradient.setColorAt(0.25, Qt::cyan);
        gradient.setColorAt(0.5, Qt::green);
        gradient.setColorAt(0.75, Qt::yellow);
        gradient.setColorAt(1.0, Qt::red);

        QGradientStops stops = gradient.stops();
        int numStops = stops.size();
        double position = adc / MAXADC;

        for(int i=0; i<numStops - 1; ++i)
        {
            if(position >= stops[i].first && position <= stops[i+1].first) {
                float t = (position - stops[i].first) / (stops[i+1].first - stops[i].first);
                return interpolateColor(stops[i].second, stops[i+1].second, t);
            }
        }

        return QColor(255, 0, 0);
    }

protected:
    void paintEvent([[maybe_unused]]QPaintEvent *event) override
    {
        QPainter painter(this);

        // Define the gradient
        QLinearGradient gradient(0, 0, 30, height());

        // Add color stops from 0 to MAXADC
        gradient.setColorAt(0.0, Qt::red);     // 0 -> red
        gradient.setColorAt(0.25, Qt::yellow); // 250 -> yellow
        gradient.setColorAt(0.5, Qt::green);   // 500 -> green
        gradient.setColorAt(0.75, Qt::cyan);   // 750 -> cyan
        gradient.setColorAt(1.0, Qt::blue);    // MAXADC -> blue

        // Fill the rectangle with the gradient
        painter.fillRect(0, 0, 30, height(), gradient);

        drawLabels(painter, gradient);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        update();
    }

private:
    QColor interpolateColor(const QColor &c1, const QColor &c2, float t) const {
        int r = static_cast<int>(c1.red() + t * (c2.red() - c1.red()));
        int g = static_cast<int>(c1.green() + t * (c2.green() - c1.green()));
        int b = static_cast<int>(c1.blue() + t * (c2.blue() - c1.blue()));

        return QColor(r, g, b);
    }

    void drawLabels(QPainter &painter, const QLinearGradient &gradient) const {
        QGradientStops stops = gradient.stops();
        QFontMetrics metrics(painter.font());
        int numStops = stops.size();

        for(int i=0; i<numStops; i++) {
            double position = stops[i].first;
            int y = static_cast<int>(position * height());

            QString label = QString::number(static_cast<int>( (1.-position) * MAXADC));
            //int textWidth = metrics.horizontalAdvance(label);
            painter.setPen(Qt::black);

            if(i == 0) {
                painter.drawLine(30, y+2, 60, y+2); // 2 pixel to accomodate for line thickness
                painter.drawText(60, y + metrics.ascent(), label);
            }
            else if(i == numStops - 1) {
                painter.drawLine(30, y-2, 60, y-2);
                painter.drawText(60, y - metrics.ascent(), label);
            }
            else {
                painter.drawLine(30, y, 45, y);
                painter.drawText(60, y, label);
            }
        }
    }

    // max adc
    double MAXADC = 1000;
};

#endif
