#include "PedestalPlotWindow.h"
#include "HistoWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// ctor

PedestalPlotWindow::PedestalPlotWindow(const QString &pedPath, QWidget *parent)
    : QMainWindow(parent)
{
    resize(1100, 800);
    setWindowTitle(tr("Pedestal Plots -- ") + pedPath);

    if(!LoadPedestal(pedPath)) {
        QMessageBox::warning(this, tr("Pedestal Plots"),
                tr("Failed to parse pedestal file:\n%1").arg(pedPath));
        // still build the UI -- it just shows empty plots, easier to debug
    }
    BuildUi();
    PopulatePageCombos();
    if(!m_apvs.empty()) {
        ShowOffsetPage(0);
        ShowNoisePage(0);
    }
}

////////////////////////////////////////////////////////////////////////////////
// pedestal parser (mirrors scripts/plot_pedestal.cpp lines 35-61)

bool PedestalPlotWindow::LoadPedestal(const QString &path)
{
    std::ifstream f(path.toStdString());
    if(!f.is_open()) return false;

    m_apvs.clear();
    APVPed cur;
    bool have_apv = false;
    std::string line;
    while(std::getline(f, line)) {
        if(line.find("APV") != std::string::npos) {
            // flush previous APV
            if(have_apv && (cur.offset.size() == 128 || cur.noise.size() == 128))
                m_apvs.push_back(cur);
            cur = APVPed();
            std::istringstream iss(line);
            std::string tag;
            iss >> tag >> cur.crate >> cur.slot >> cur.mpd >> cur.adc;
            have_apv = true;
        } else if(have_apv) {
            std::istringstream iss(line);
            int strip = -1;
            double offset = 0.0, noise = 0.0;
            if(iss >> strip >> offset >> noise) {
                cur.offset.push_back(offset);
                cur.noise.push_back(noise);
            }
        }
    }
    // flush the last APV
    if(have_apv && (cur.offset.size() == 128 || cur.noise.size() == 128))
        m_apvs.push_back(cur);

    return !m_apvs.empty();
}

////////////////////////////////////////////////////////////////////////////////
// UI

void PedestalPlotWindow::BuildUi()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *vMain = new QVBoxLayout(central);

    QTabWidget *tabs = new QTabWidget(central);

    // ----- Offset tab -----
    QWidget *offsetTab = new QWidget(tabs);
    QVBoxLayout *vOff = new QVBoxLayout(offsetTab);
    QWidget *topOff = new QWidget(offsetTab);
    QHBoxLayout *hOff = new QHBoxLayout(topOff);
    hOff->setContentsMargins(0, 0, 0, 0);
    hOff->addWidget(new QLabel(tr("Page:"), topOff));
    m_offsetCombo = new QComboBox(topOff);
    m_offsetCombo->setMinimumWidth(160);
    hOff->addWidget(m_offsetCombo);
    hOff->addStretch(1);
    vOff->addWidget(topOff);
    m_offsetWidget = new HistoWidget(offsetTab);
    vOff->addWidget(m_offsetWidget, 1);
    tabs->addTab(offsetTab, tr("Offset"));

    // ----- Noise tab -----
    QWidget *noiseTab = new QWidget(tabs);
    QVBoxLayout *vNoi = new QVBoxLayout(noiseTab);
    QWidget *topNoi = new QWidget(noiseTab);
    QHBoxLayout *hNoi = new QHBoxLayout(topNoi);
    hNoi->setContentsMargins(0, 0, 0, 0);
    hNoi->addWidget(new QLabel(tr("Page:"), topNoi));
    m_noiseCombo = new QComboBox(topNoi);
    m_noiseCombo->setMinimumWidth(160);
    hNoi->addWidget(m_noiseCombo);
    hNoi->addStretch(1);
    vNoi->addWidget(topNoi);
    m_noiseWidget = new HistoWidget(noiseTab);
    vNoi->addWidget(m_noiseWidget, 1);
    tabs->addTab(noiseTab, tr("Noise"));

    vMain->addWidget(tabs, 1);
    setCentralWidget(central);

    // light style consistent with the rest of the GUI
    setStyleSheet(R"(
            QMainWindow { background: #FFFFEF; }
            QTabWidget::pane { border: 1px solid #D0D3D8; background: #FFFFFF; }
            QTabBar::tab {
                background: #E8EAED; padding: 4px 12px; margin: 2px;
                border-radius: 4px;
            }
            QTabBar::tab:selected { background: #FFFFFF; }
            )");

    connect(m_offsetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PedestalPlotWindow::ShowOffsetPage);
    connect(m_noiseCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PedestalPlotWindow::ShowNoisePage);
}

void PedestalPlotWindow::PopulatePageCombos()
{
    const int n = static_cast<int>(m_apvs.size());
    const int nPages = (n + kPerPage - 1) / kPerPage;

    m_offsetCombo->blockSignals(true);
    m_noiseCombo ->blockSignals(true);
    m_offsetCombo->clear();
    m_noiseCombo ->clear();
    for(int p = 0; p < nPages; ++p) {
        const QString label = tr("page %1 / %2").arg(p + 1).arg(nPages);
        m_offsetCombo->addItem(label);
        m_noiseCombo ->addItem(label);
    }
    m_offsetCombo->blockSignals(false);
    m_noiseCombo ->blockSignals(false);
}

////////////////////////////////////////////////////////////////////////////////
// build one page-worth of PlotData and hand it to the HistoWidget

void PedestalPlotWindow::Plot(HistoWidget *widget, int pageIdx, bool offsetMode)
{
    if(!widget) return;

    std::vector<HistoWidget::PlotData> plots;
    const int base = pageIdx * kPerPage;
    const int n    = static_cast<int>(m_apvs.size());

    for(int i = 0; i < kPerPage; ++i) {
        const int idx = base + i;
        if(idx >= n) break;
        const APVPed &apv = m_apvs[idx];
        const std::vector<double> &v = offsetMode ? apv.offset : apv.noise;
        if(v.empty()) continue;

        HistoWidget::PlotData pd;
        pd.type = HistoWidget::PlotData::Plot1D;
        char buf[160];
        std::snprintf(buf, sizeof(buf), "%s crate:%d mpd:%d adc:%d",
                      offsetMode ? "offset" : "noise",
                      apv.crate, apv.mpd, apv.adc);
        pd.title = buf;

        // basic stats for the on-plot box
        double sum = 0.0, sum2 = 0.0;
        for(double x : v) { sum += x; sum2 += x * x; }
        const double mean = sum / v.size();
        const double var  = std::max(0.0, sum2 / v.size() - mean * mean);
        const double rms  = std::sqrt(var);
        std::snprintf(buf, sizeof(buf), "Entries %zu", v.size());
        pd.stats.push_back(buf);
        std::snprintf(buf, sizeof(buf), "Mean %.3g", mean);
        pd.stats.push_back(buf);
        std::snprintf(buf, sizeof(buf), "Std Dev %.3g", rms);
        pd.stats.push_back(buf);

        // Pad the X range with zero bins on either side so the first/last
        // strip don't sit against the plot frame. Strip 0 lands at x=0
        // (bin index = kPad), strip 127 at x=127. Bins outside [0, 127]
        // are filled with 0 and provide a visual margin.
        constexpr int kPad      = 5;    // bins on the left of strip 0
        constexpr int kPadRight = 13;   // bins on the right of strip 127
        const int nStrips = static_cast<int>(v.size());
        const int nBins   = nStrips + kPad + kPadRight;
        pd.nx   = nBins;
        pd.xMin = -kPad;
        pd.xMax = static_cast<double>(nStrips + kPadRight);
        pd.y.assign(nBins, 0.0);
        for(int s = 0; s < nStrips; ++s)
            pd.y[s + kPad] = v[s];
        plots.push_back(std::move(pd));
    }

    widget->DrawCanvas(plots, kRows, kCols);
}

void PedestalPlotWindow::ShowOffsetPage(int idx)
{
    Plot(m_offsetWidget, idx, /*offsetMode=*/true);
}

void PedestalPlotWindow::ShowNoisePage(int idx)
{
    Plot(m_noiseWidget, idx, /*offsetMode=*/false);
}
