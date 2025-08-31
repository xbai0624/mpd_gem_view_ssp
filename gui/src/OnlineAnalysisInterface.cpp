#include "OnlineAnalysisInterface.h"

#include <QLayout>

OnlineAnalysisInterface::OnlineAnalysisInterface(QWidget *parent) : QMainWindow(parent)
{
    resize(1200, 800);
    setWindowTitle("Online Analysis");

    AddMenuBar();
}

OnlineAnalysisInterface::~OnlineAnalysisInterface()
{
}

void OnlineAnalysisInterface::AddMenuBar()
{
    pMenuBar = new QMenuBar();

    // show analysis configs
    pShowAnalysisConfig = new QMenu("Configure");
    pShow = new QAction("Show", this);
    pShowAnalysisConfig -> addAction(pShow);
    pMenuBar -> addMenu(pShowAnalysisConfig);

    // start analysis
    pAnalyze = new QMenu("Analyze");
    pStartAnalysis = new QAction("Go", this);
    pAnalyze -> addAction(pStartAnalysis);
    pMenuBar -> addMenu(pAnalyze);

    // show menu bar
    this -> layout() -> setMenuBar(pMenuBar);
}
