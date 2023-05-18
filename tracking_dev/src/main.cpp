#include <iostream>
#include <QApplication>
#include "Viewer.h"
#include "Tracking.h"

int main(int argc, char* argv[])
{
    // test
    /*
    Tracking *tracking = new Tracking();
    tracking -> UnitTest();
    return 0;
    */

    QApplication app(argc, argv);

    tracking_dev::Viewer *view = new tracking_dev::Viewer();
    view -> resize(1000, 600);
    view -> show();

    return app.exec();
}
