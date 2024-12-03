#include "Viewer.h"

#include <QApplication>
#include <TApplication.h>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    TApplication root_app("root qt", &argc, argv);

    Viewer *view = new Viewer();
    view->resize(800, 550);
    view->show();

    QObject::connect(qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));
    return app.exec();
}
