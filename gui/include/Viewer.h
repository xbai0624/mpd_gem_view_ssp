#ifndef VIEWER_H
#define VIEWER_H

#include "QMainCanvas.h"
#include "ComponentsSchematic.h"
#include "GEMAnalyzer.h"
#include "GEMReplay.h"
#include "APVStripMapping.h"
#include "ConfigObject.h"
#include "HistoWidget.h"
#include "Detector2DView.h"
#include "OnlineAnalysisInterface.h"

#include <QMainWindow>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <QLineEdit>
#include <QTextEdit>

#include <vector>
#include <string>
#include <deque>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget *parent = 0);
    ~Viewer() {}

    void InitGui();
    void AddMenuBar();

    // used to draw a schematic of detector apv setup
    // highly dependent on each setup, use is optional
    void InitComponentsSchematic();

    void InitLayout();
    void InitCtrlInterface();
    void InitLeftTab();
    void InitLeftView();
    void InitRightView();

    // init detector analyzers
    void InitGEMAnalyzer();

    bool FileExist(const char* path);

    // setters
    void SetNumberOfTabs();
    void ParsePedestalsOutputPathFromEvioFile();

public slots:
    void SetFile(const QString &);
    void SetFileSplitMax(const QString &);
    void SetFileSplitMin(const QString &);
    void SetRootFileOutputPath(const QString &);
    void SetPedestalOutputPath(const QString &);
    void SetPedestalMaxEvents(const QString &);
    void SetCommonModeOutputPath(const QString &);
    void SetPedestalInputPath(const QString &);
    void SetCommonModeInputPath(const QString &);
    void ChoosePedestal();
    void ChooseCommonMode();
    void DrawEvent(int);
    void DrawGEMRawHistos(int);
    void DrawGEMOnlineHits(int);
    void OpenFile();
    void GeneratePedestal_obsolete();
    void GeneratePedestal();
    void ReplayHit();
    void ReplayCluster();
    void OpenOnlineAnalysisInterface();
    void SaveCurrentEvent();

private:
    // layout
    QVBoxLayout *pMainLayout;
    QHBoxLayout *pDrawingLayout;
    QVBoxLayout *pLeftLayout;
    QVBoxLayout *pRightLayout;

    // contents to show
    ComponentsSchematic *componentsView;    // detector setup
    QWidget *pDrawingArea;                  // whole drawing area (left + right)
    QWidget *pLeft;                         // left area
    QWidget *pRight;                        // right area
    QTabWidget *pLeftTab;                   // tab for the left side area
    //std::vector<QMainCanvas*> vTabCanvas; // tab contents, using cern root
    std::vector<HistoWidget*> vTabCanvas;   // tab contents, use self-implemented HistoWidgets
    QMainCanvas *pRightCanvas;              // right side canvas
    QWidget *pRightCtrlInterface;           // the control interface on right side
    // online hits
    std::vector<HistoWidget*> vTabCanvasOnlineHits; // tab contents, for drawing online hits
    bool reload_pedestal_for_online = true;

    // menu bar
    QMenu *pMenu;
    QMenuBar *pMenuBar;
    QMenu *pOnlineAnalysis;
    QAction *pOpenAnalysisInterface;
    // open file (line input)
    QLineEdit *file_indicator;
    // print info on the gui
    QTextEdit *pLogBox;

    // show detector 2d strips for eye-ball tracking
    Detector2DView *det_view;

    // online analysis interface window
    OnlineAnalysisInterface *winOnlineInterface;

    // number of tabs
    int nTab = 12; // number of tabs for apv raw histos
    int nTabOnlineHits = 12; // number of tabs for online hits

    // GEM analzyer
    GEMAnalyzer *pGEMAnalyzer;
    // evio file to be analyzed
    std::string fFile = "gui/data/gem_cleanroom_1440.evio.0";
    // pedestal output default path
    std::string fPedestalOutputPath = "database/gem_ped.dat";
    std::string fCommonModeOutputPath = "database/CommonModeRange.txt";
    uint32_t fPedestalMaxEvents = 5000;
    // pedestal input (for data analysis) default path
    std::string fPedestalInputPath;
    std::string fCommonModeInputPath;

    // gem replay
    GEMReplay *pGEMReplay;
    std::string fRootFileSavePath = "./gem_replay.root";
    int fFileSplitEnd = -1;
    int fFileSplitStart = 0;

private:
    // section for GEM_Viewer status
    int event_number_checked = 0;
    int current_event_number = 0;
    size_t max_cache_events = 500;
    std::deque<std::map<APVAddress, std::vector<int>>> event_cache;
    std::deque<std::map<APVAddress, APVDataType>> event_flag_cache;

    // a text parser
    ConfigObject txt_parser;

public:
    template<typename T> void minimum_qt_unit_height(T b)
    {
        b -> setMinimumHeight(10);
    }
    template<typename T, typename... Args> void minimum_qt_unit_height(T b, Args... args) {
        minimum_qt_unit_height(b);
        minimum_qt_unit_height(args...);
    }
};

#endif
