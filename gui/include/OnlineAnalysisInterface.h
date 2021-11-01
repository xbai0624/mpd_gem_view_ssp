#ifndef ONLINE_ANALYSIS_INTERFACE_H
#define ONLINE_ANALYSIS_INTERFACE_H

////////////////////////////////////////////////////////////////////////////////
// a popup window for online analysis

#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

class OnlineAnalysisInterface : public QMainWindow
{
    Q_OBJECT

public:
    OnlineAnalysisInterface(QWidget *parent=nullptr);
    ~OnlineAnalysisInterface();

    void AddMenuBar();

private:
    QMenuBar *pMenuBar;
    QMenu *pShowAnalysisConfig;
    QAction *pShow;
    QMenu *pAnalyze;
    QAction *pStartAnalysis;
};

#endif
