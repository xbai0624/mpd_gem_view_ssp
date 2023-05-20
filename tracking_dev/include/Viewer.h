#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include "histos.hpp"

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
class QLineEdit;
class QSpinBox;
class TRandom;

namespace tracking_dev {

class AbstractDetector;
class Detector2DItem;
class Detector2DView;
class Tracking;
class TrackingDataHandler;

#define NDET_SIM 4
//#define N_BACKGROUND 178 // 1e9 combinations
#define N_BACKGROUND 0

class Viewer : public QWidget
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

        void ProcessTrackingResult();
        bool ProcessRawGEMResult();

public slots:
        void DrawEvent(int);
        void FillEventHistos();
        void Replay50K();
        void OpenFile();
        void ProcessNewFile(const QString &);

public:
        // a helper
        void ShowGridHitStat();

private:
        AbstractDetector *fDet[1000]; // max 1000 detector

        Detector2DItem *fDet2DItem[1000]; // max 1000 detector
        Detector2DView *fDet2DView;
        QSpinBox *btn_next;
        QPushButton *btn_50K;
        QPushButton *btn_open_file;
        QLabel *label_counter;
        QLineEdit *label_file;
        QVBoxLayout *global_layout;

        TRandom *gen;

        Tracking *tracking;
        TrackingDataHandler *tracking_data_handler;

        // histos
        histos::HistoManager<> hist_m;

        int fEventNumber = 0;
        std::string evio_file;

        int NDetector_Implemented = 0;

        // for toy model
        double fXOffset[NDET_SIM] = {0};
        double fYOffset[NDET_SIM] = {0};

        //double fXOffset[4] = {0, 2., -1., 3.};
        //double fYOffset[4] = {0, 2., -1., 3.};
};

};

#endif
