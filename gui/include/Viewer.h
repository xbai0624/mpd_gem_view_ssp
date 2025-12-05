#ifndef VIEWER_H
#define VIEWER_H

//#include "QMainCanvas.h"
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
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>

#include <vector>
#include <string>
#include <deque>

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer(QWidget *parent = 0);
    ~Viewer() {}

    void LoadMappingFile();

    void InitGuiInterface();
    void createMenuBar();

    QWidget* createTopToolbar();

    QWidget* createDetectorPanel();
    QWidget* createRawFramesView(QWidget*);
    QWidget* createOnlineHitsView(QWidget*);
    QWidget* createDetector2DStripsView(QWidget*);

    QWidget* createSettingsPanel();
    QWidget* createPedestalCommonModePage();
    QWidget* createMappingFilePage();
    QWidget* createReplayPage();
    QWidget* createAdvancedPage();

    QWidget* createSystemLogPanel();

    // used to draw a schematic of detector apv setup
    // highly dependent on each setup, use is optional
    void InitComponentsSchematic();

    // init detector analyzers
    void InitGEMAnalyzer();

    bool FileExist(const char* path);

    // setters
    void SetNumberOfTabs();
    void ParsePedestalsOutputPathFromEvioFile();

public slots:
    void SetFile(const QString &);
    void SetFileSplitMax(const int &);
    void SetFileSplitMin(const int &);
    void SetRootFileOutputPath(const QString &);
    void SetPedestalOutputPath(const QString &);
    void SetPedestalMaxEvents(const int &);
    void SetCommonModeOutputPath(const QString &);
    void SetPedestalInputPath(const QString &);
    void SetCommonModeInputPath(const QString &);
    void ReloadPedestal();
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

public:
    template<typename T> void minimum_qt_unit_height(T b)
    {
        b -> setMinimumHeight(10);
    }
    template<typename T, typename... Args> void minimum_qt_unit_height(T b, Args... args) {
        minimum_qt_unit_height(b);
        minimum_qt_unit_height(args...);
    }

private:
    QComboBox *m_fileCombo; // file selector
    QComboBox *m_viewCombo; // view selector
    QSpinBox *m_eventSpin;  // event number
    QLineEdit *m_pedOut; // pedestal path
    QLineEdit *m_cmOut; // common mode path

    QPlainTextEdit *m_logEdit;

    // contents to show
    ComponentsSchematic *componentsView;    // detector setup
    std::vector<HistoWidget*> vTabCanvas;   // tab contents, use self-implemented HistoWidgets
    // online hits
    std::vector<HistoWidget*> vTabCanvasOnlineHits; // tab contents, for drawing online hits
    bool reload_pedestal_for_online = true;

    // show detector 2d strips for eye-ball tracking
    Detector2DView *det_view;

    // online analysis interface window
    OnlineAnalysisInterface *winOnlineInterface;

    // number of tabs
    int nTab = 12; // number of tabs for apv raw histos
    int nTabOnlineHits = 12; // number of tabs for online hits

    // GEM analzyer
    GEMAnalyzer *pGEMAnalyzer;
    std::string fFile = "gui/data/gem_cleanroom_1440.evio.0";
    std::string fPedestalOutputPath = "database/gem_ped.dat";
    std::string fCommonModeOutputPath = "database/CommonModeRange.txt";
    uint32_t fPedestalMaxEvents = 5000;
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
};

#endif
