#ifndef VIEWER_H
#define VIEWER_H

#include <QMainWindow>
#include "histos.hpp"
#include "tracking_struct.h"
#include <thread>

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
class QLineEdit;
class QSpinBox;
class QToolBar;
class TRandom;

namespace tracking_dev {

class VirtualDetector;
class Detector2DHitItem;
class Detector2DHitView;
class IsometricView;
class Tracking;
class TrackingDataHandler;
class TrackingConfigWidget;
class TrackingResultPanel;

#define NDET_SIM 4
//#define N_BACKGROUND 178 // 1e9 combinations
#define N_BACKGROUND 0

class Viewer : public QMainWindow
{
    Q_OBJECT
public:
        Viewer(QWidget *parent = 0);
        ~Viewer();

        void InitToyDetectorSetup();
        void InitGui();

        void GenerateToyTrackEvent();
        void AddToyEventBackground();
        void ClearPrevEvent();
        void GetOneFermiTrack(int i);

        void ProcessTrackingResult();
        void ProcessRawGEMResult();
        void UpdateStatusBar(int event_ordinal);
        void UpdateResultHistos();   // refresh right-panel histos after Replay 50K

public slots:
        void DrawEvent(int);
        void FillEventHistos();
        void Replay50K();
        void OpenFile();
        void OpenSettings();        // Settings menu -> config pop-up
        void ProcessNewFile(const QString &);

signals:
        void ReplayProgress(int events, double seconds_per_1000);
        void ReplayFinished(int events, double total_seconds);

private slots:
        void UpdateReplayProgress(int events, double seconds_per_1000);
        void FinishReplay50K(int events, double total_seconds);

public:
        // a helper
        void ShowGridHitStat();
        void LoadFermiData();

private:
        // InitGui sub-steps (pure UI assembly, one job each)
        void InitDetectorViews();
        void ApplyTheme();
        void createMenuBar();
        QWidget* createTopToolbar();
        QWidget* createCentralArea();
        void createStatusBar();
        void InitConnections();

        void RunReplay50KWorker();
        void FinalizeReplay50KHistos();
        void SetReplayControlsEnabled(bool enabled);
        //VirtualDetector *fDet[1000]; // max 1000 detector
        // <layer_id, detector_pointer*>
        std::unordered_map<int, VirtualDetector*> fDet;

        //Detector2DHitItem *fDet2DItem[1000]; // max 1000 detector
        // <layer_id, detector_pointer*>
        std::unordered_map<int, Detector2DHitItem*> fDet2DItem;

        Detector2DHitView *fDet2DView;
        // sibling 3D isometric view (stacked layers, hits + track line)
        IsometricView *fIsoView;
        TrackingConfigWidget *config_panel = nullptr;
        QWidget *settings_window = nullptr;   // pop-up holding config_panel
        TrackingResultPanel *result_panel = nullptr;  // right panel: result histograms
        QSpinBox *btn_next;                   // jump-to event spinbox
        QPushButton *btn_prev = nullptr;      // step back one event
        QPushButton *btn_next_step = nullptr; // step forward one event
        QPushButton *btn_50K;
        QPushButton *btn_open_file;
        QLineEdit *label_file;
        QVBoxLayout *global_layout;

        // status bar readout
        QLabel *m_statEvent  = nullptr;
        QLabel *m_statTracks = nullptr;
        QLabel *m_statChi2   = nullptr;
        QLabel *m_statTiming = nullptr;   // Replay-50K progress / timing

        // per-event status cache so stepping back (Prev) shows that event's
        // tracking result (the backward path only redraws cached plots and
        // does not re-run tracking). Indexed by event ordinal - 1.
        std::vector<int>    m_hist_ntracks;
        std::vector<int>    m_hist_ncandidates;
        std::vector<double> m_hist_chi2;
        std::vector<bool>   m_hist_found;

        bool m_replay50KRunning = false;
        std::thread m_replay50KThread;

        TRandom *gen;

        Tracking *tracking;
        TrackingDataHandler *tracking_data_handler;

        // histos
        histos::HistoManager<> hist_m;

        int fEventNumber = 0;
        std::string evio_file;

        int NDetector_Implemented = 0;
        std::vector<int> layer_ids;

        // for toy model
        double fXOffset[NDET_SIM] = {0};
        double fYOffset[NDET_SIM] = {0};

        //double fXOffset[4] = {0, 2., -1., 3.};
        //double fYOffset[4] = {0, 2., -1., 3.};

        std::vector<std::vector<point_t>> total_tracks;
};

};

#endif
