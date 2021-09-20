#include <QApplication>
#include <TApplication.h>
#include "FadcDataViewer.h"

int main([[maybe_unused]]int argc, [[maybe_unused]]char* argv[])
{
    QApplication app(argc, argv);
    TApplication root_app("root qt", &argc, argv);

    FadcDataViewer *viewer = new FadcDataViewer(nullptr);
    viewer -> SetFile("data/SoLIDCer516.dat.0");
    viewer -> resize(1600, 800);
    viewer -> show();

    // qApp: a global pointer referring to the unique application object
    QObject::connect(qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));
    return app.exec();
}
