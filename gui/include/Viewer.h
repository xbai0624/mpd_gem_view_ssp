#ifndef VIEWER_H
#define VIEWER_H

//#include "QMainCanvas.h"
#include "GEMAnalyzer.h"
#include "GEMReplay.h"
#include "APVStripMapping.h"
#include "ConfigObject.h"
#include "HistoWidget.h"
#include "Detector2DView.h"
#include "OnlineAnalysisInterface.h"
#include "PedestalPlotWindow.h"

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

class QTimer;
class QRadioButton;
// Online (ET) monitoring is optional; the wrapper type is only referenced
// when the GUI is built with CONFIG+=et (HAVE_ET). Forward-declared so the
// header stays ET-free.
namespace online_monitor { class OnlineMonitor; }

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
    QWidget* createPRadSetupPage();
    QWidget* createPedestalCommonModePage();
    QWidget* createMappingFilePage();
    QWidget* createReplayPage();
    QWidget* createAdvancedPage();

    QWidget* createSystemLogPanel();

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
    void PlotPedestal();
    void ReplayHit();
    void ReplayCluster();
    void OpenOnlineAnalysisInterface();
    void SaveCurrentEvent();
    void Prepare2DGeoHits(GEMDetector *);

    // online (ET) monitoring control. Bodies are no-ops unless built with
    // HAVE_ET, but the slots are always declared so moc/linkage is stable.
    void ToggleOnline(bool on);   // start/stop the live ET feed
    void PollOnlineEvent();       // timer tick: pull + draw one ET event

signals:
    void onlineHitsDrawn(const QMap<int, QVector<QPointF>>&);

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
    std::vector<HistoWidget*> vTabCanvas;   // tab contents, use self-implemented HistoWidgets
    // online hits
    std::vector<HistoWidget*> vTabCanvasOnlineHits; // tab contents, for drawing online hits
    bool reload_pedestal_for_online = true;

    // show detector 2d strips for eye-ball tracking
    Detector2DView *det_view;

    // online analysis interface window
    OnlineAnalysisInterface *winOnlineInterface;

    // pedestal plot popup window (lazy-created on first PlotPedestal())
    PedestalPlotWindow *winPedestalPlot = nullptr;

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
    QMap<int, QVector<QPointF>> detector_2d_geo_hits;

    // a text parser
    ConfigObject txt_parser;

    // ---- online (ET) monitoring ----
    bool online_mode = false;          // true while a live ET feed is running
    int  online_event_counter = 0;     // ever-increasing event index for online
    QTimer *online_timer = nullptr;    // drives periodic event pulls
    // Online/Offline radio buttons live in the Advanced page; selecting
    // "Online Mode" starts the ET feed (disabled when built without ET).
    QRadioButton *m_cbOnline  = nullptr;
    QRadioButton *m_cbOffline = nullptr;
#ifdef HAVE_ET
    online_monitor::OnlineMonitor *pOnlineMonitor = nullptr;
#endif
    // connection params, loaded from config/online.conf
    std::string fOnlineEtFile  = "/tmp/et_sys_gem";
    std::string fOnlineHost    = "localhost";
    std::string fOnlineStation = "gem_monitor";
    int fOnlinePort   = 11111;
    int fOnlinePollMs = 200;
};

#endif
