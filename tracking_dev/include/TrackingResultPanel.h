/*******************************************************************************
 * TrackingResultPanel
 *
 *   The right-hand panel that shows the tracking RESULT histograms produced
 *   by a Replay-50K pass. A themed QToolBox accordion of histogram sections,
 *   reusing the pure-Qt HistoItem (1D) / HistoItem2D (2D) widgets from gui/.
 *   The caller extracts bin data from ROOT TH1/TH2 and hands plain arrays to
 *   AddHisto1D / AddHisto2D, so this class stays ROOT-free.
 *
 *   (Named for its purpose -- showing tracking results -- to avoid confusion
 *   with the general-purpose HistoWidget.)
 *
 *   Plots are grouped into collapsible sections (one open at a time) to keep
 *   the narrow right-hand panel compact instead of one long scroll. Each
 *   section owns a QGraphicsView in which its few items are stacked
 *   vertically at a fixed per-plot height.
 ******************************************************************************/

#ifndef TRACKING_RESULT_PANEL_H
#define TRACKING_RESULT_PANEL_H

#include <QWidget>

#include <string>
#include <vector>

class QToolBox;
class QGraphicsScene;
class QGraphicsView;
class QGraphicsItem;

namespace tracking_dev {

class TrackingResultPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TrackingResultPanel(QWidget *parent = nullptr);

    void Clear();                                 // remove all sections
    void AddSection(const std::string &name);     // start a new collapsible page

    // add to the current (most recently added) section
    void AddHisto1D(const std::string &title,
                    const std::vector<std::string> &stats,
                    int nbins, double xMin, double xMax,
                    const std::vector<double> &y);

    void AddHisto2D(const std::string &title,
                    const std::vector<std::string> &stats,
                    int nx, double xMin, double xMax,
                    int ny, double yMin, double yMax,
                    const std::vector<double> &z);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    struct Section {
        QGraphicsScene *scene = nullptr;
        QGraphicsView  *view  = nullptr;
        std::vector<QGraphicsItem*> items;        // owned by scene
    };

    void RelayoutSection(Section &s);

    QToolBox *m_toolbox = nullptr;
    std::vector<Section> m_sections;
    int m_rowH = 240;                             // per-plot height (px)
};

};

#endif
