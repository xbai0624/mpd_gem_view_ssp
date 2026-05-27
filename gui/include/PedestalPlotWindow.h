/*******************************************************************************
 * PedestalPlotWindow
 *
 *   Popup window that plots per-APV offset and noise distributions from a
 *   pedestal text file (database/gem_ped*.dat). Mirrors
 *   scripts/plot_pedestal.cpp's behaviour but renders into the in-GUI
 *   HistoWidget grid instead of ROOT canvases.
 *
 *   Two top-level tabs ("Offset" / "Noise"); each tab has a 4x4 page of 1D
 *   plots driven by a page combo so all APVs are reachable without
 *   scrolling.
 ******************************************************************************/

#ifndef PEDESTAL_PLOT_WINDOW_H
#define PEDESTAL_PLOT_WINDOW_H

#include <QMainWindow>
#include <QString>

#include <string>
#include <vector>

class QComboBox;
class HistoWidget;

class PedestalPlotWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PedestalPlotWindow(const QString &pedPath,
                                QWidget *parent = nullptr);
    ~PedestalPlotWindow() override = default;

private slots:
    void ShowOffsetPage(int idx);
    void ShowNoisePage(int idx);

private:
    struct APVPed {
        int crate = 0, slot = 0, mpd = 0, adc = 0;
        std::vector<double> offset;   // size 128
        std::vector<double> noise;    // size 128
    };

    bool LoadPedestal(const QString &path);
    void BuildUi();
    void PopulatePageCombos();
    void Plot(HistoWidget *widget, int pageIdx, bool offsetMode);

    std::vector<APVPed> m_apvs;
    QComboBox   *m_offsetCombo  = nullptr;
    QComboBox   *m_noiseCombo   = nullptr;
    HistoWidget *m_offsetWidget = nullptr;
    HistoWidget *m_noiseWidget  = nullptr;

    static constexpr int kRows = 4;
    static constexpr int kCols = 4;
    static constexpr int kPerPage = kRows * kCols;
};

#endif
