#ifndef QMAINCANVAS_H
#define QMAINCANVAS_H

#include "QRootCanvas.h"
#include "MPDDataStruct.h"

#include <QVBoxLayout>
#include <QTimer>

#include <TCanvas.h>
#include <vector>
#include <TH1I.h>

////////////////////////////////////////////////////////////////////////////////
// a wrapper class for QRootCanvas, this is necessary, b/c in this wrapper class,
// a root timer was implemented: handle_root_events()

class QMainCanvas : public QWidget
{
    Q_OBJECT

public:
    QMainCanvas(QWidget *parent = 0);
    ~QMainCanvas() {}
    TCanvas *GetCanvas();

    void Refresh();

    // helpers
    void GenerateHistos(const std::vector<std::vector<int>> &hists,
            const std::vector<APVAddress> &addr);

public slots:
    void DrawCanvas(const std::vector<TH1I*> &, int, int);
    void DrawCanvas(const std::vector<std::vector<int>> &, 
            const std::vector<APVAddress> &addr, int, int);
    void DrawCanvas(const std::string &s, const std::vector<float> &c);

    void handle_root_events();

signals:
    void ItemSelected();
    void ItemDeSelected();

protected:
    virtual void mousePressEvent(QMouseEvent *);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *e);

protected:
    QRootCanvas *fCanvas;
    QVBoxLayout *layout;

    QTimer *fRootTimer;
    TCanvas *fRootCanvas = nullptr;

    std::vector<TH1I*> vHContents;
};

#endif
