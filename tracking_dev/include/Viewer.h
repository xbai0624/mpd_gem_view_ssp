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

#define NDET 4
//#define N_BACKGROUND 178 // 1e9 combinations
#define N_BACKGROUND 0

class Viewer : public QWidget
{
    Q_OBJECT
public:
        Viewer(QWidget *parent = 0);
        ~Viewer();

        void InitDetectorSetup();
        void InitGui();

        void GenerateTrackEvent();
        void AddEventBackground();
        void ClearPrevEvent();

        void ProcessTrackingResult();
        bool ProcessRawGEMResult();

public slots:
        void GenerateEvent();
        void FillEventHistos();
        void Replay50K();
        void OpenFile();
        void ProcessNewFile(const QString &);

public:
        // a helper
        void ShowGridHitStat();

private:
        AbstractDetector *fDet[NDET];

        Detector2DItem *fDet2DItem[NDET];
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

        double fXOffset[4] = {0, 0., 0., 0.};
        double fYOffset[4] = {0, 0., 0., 0.};

        //double fXOffset[4] = {0, 2., -1., 3.};
        //double fYOffset[4] = {0, 2., -1., 3.};
};

};

#endif
