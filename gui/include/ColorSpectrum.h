#ifndef COLOR_SPECTRUM_H
#define COLOR_SPECTRUM_H

/*
 * Here defines a color bar
 * color bar range from 0 to 1
 * and also routines for converting a double number (0, 1) to a QColor (RGB) that 
 * corresponds the correct position in the color bar
 */

// for drawing color bar
#include <QPixmap>

class ColorSpectrum 
{
public:
    ColorSpectrum();
    ~ColorSpectrum();

    // convert a double (0,1) to a RGB color
    QColor toColor(double f);

    // a vertical color bar
    QPixmap rainbowVertical(int w, int h);

    // a horizontal color bar
    QPixmap rainbowHorizontal(int w, int h);

private:
};

#endif
