#include "ColorSpectrum.h"
#include <QPainter>
#include <cmath>

// original code from: https://www.particleincell.com/2014/colormap/

ColorSpectrum::ColorSpectrum()
{
    // place holder
}

ColorSpectrum::~ColorSpectrum()
{
    // place holder
}

QColor ColorSpectrum::toColor(double f)
{
    double v1 = (1 - f)/0.25; // invert and group

    int group = floor(v1);
    int color = floor(255*(v1-group));

    int rr=0, gg=0, bb=0;
    switch(group){
        case 0:{ rr=255; gg = color; bb = 0; break;}
        case 1:{ rr=255 - color; gg = 255; bb = 0; break;}
        case 2:{ rr=0; gg = 255; bb = color; break;}
        case 3:{ rr=0; gg = 255 - color; bb = 255; break;}
        case 4:{ rr=0; gg = 0; bb = 255; break;}
    }

    return QColor(rr, gg, bb);
}

QPixmap ColorSpectrum::rainbowHorizontal(int w, int h)
{
    // horizontal color bar
    QPixmap pm(w, h);
    QPainter p(&pm);

    for(int i=0;i<w;i++){

        QColor color = toColor((double)i/(double)w);
        p.setPen(color);
        p.drawLine(i, 0, i, h);
    }
    return pm;
}

QPixmap ColorSpectrum::rainbowVertical(int w, int h)
{
    // vertical color bar
    QPixmap pm(w, h);
    QPainter p(&pm);

    for(int i=0;i<h;i++){

        QColor color = toColor((double)i/(double)h);
        p.setPen(color);
        p.drawLine(0, i, w, i);
    }
    return pm;
}
