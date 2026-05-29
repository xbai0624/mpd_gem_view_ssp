#include "TrackingResultPanel.h"
#include "HistoItem.h"
#include "HistoItem2D.h"

#include <QToolBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QPainter>

namespace tracking_dev {

TrackingResultPanel::TrackingResultPanel(QWidget *parent) : QWidget(parent)
{
    m_toolbox = new QToolBox(this);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay -> setContentsMargins(0, 0, 0, 0);
    lay -> addWidget(m_toolbox);

    setMinimumWidth(100);

    // A QToolBox only sizes the *current* page; items added to a collapsed
    // section measured a stale (half) viewport width. Re-layout a section
    // when it becomes visible so its plots fill the real width.
    connect(m_toolbox, &QToolBox::currentChanged, this, [this](int idx){
        if(idx >= 0 && idx < static_cast<int>(m_sections.size()))
            RelayoutSection(m_sections[idx]);
    });

    // match gui/src/Viewer.cpp's look (cream, themed toolbox tabs)
    setStyleSheet(R"(
            QWidget {
                 background: #FFFFEF;
            }
            QToolBox::tab {
                 background: #E8EAED;
                 border-radius: 4px;
                 padding: 4px;
                 margin: 2px;
            }
            QGraphicsView {
                 background: #FFFFFF;
                 border: 1px solid #D0D3D8;
                 border-radius: 4px;
            }
            )");
}

void TrackingResultPanel::Clear()
{
    // Block signals while clearing: removeItem() changes the current page and
    // emits currentChanged(), whose slot would RelayoutSection() a section
    // whose scene/view we are deleting here -> dangling-pointer crash.
    // Drop m_sections first so nothing references the soon-deleted pages.
    m_toolbox->blockSignals(true);
    m_sections.clear();
    while(m_toolbox->count() > 0) {
        QWidget *page = m_toolbox->widget(0);
        m_toolbox->removeItem(0);
        delete page;
    }
    m_toolbox->blockSignals(false);
}

void TrackingResultPanel::AddSection(const std::string &name)
{
    QWidget *page = new QWidget(m_toolbox);
    QVBoxLayout *pl = new QVBoxLayout(page);
    pl -> setContentsMargins(2, 2, 2, 2);

    Section s;
    s.scene = new QGraphicsScene(page);
    s.view  = new QGraphicsView(s.scene, page);
    s.view -> setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    s.view -> setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s.view -> setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    s.view -> setAlignment(Qt::AlignTop | Qt::AlignLeft);
    s.view -> viewport() -> setAttribute(Qt::WA_AcceptTouchEvents, false);
    pl -> addWidget(s.view);

    m_toolbox -> addItem(page, QString::fromStdString(name));
    m_sections.push_back(s);
}

void TrackingResultPanel::AddHisto1D(const std::string &title,
                                     const std::vector<std::string> &stats,
                                     int nbins, double xMin, double xMax,
                                     const std::vector<double> &y)
{
    if(m_sections.empty()) return;
    Section &s = m_sections.back();

    HistoItem *item = new HistoItem();

    QVector<QPair<double, double>> pts;
    pts.reserve(static_cast<int>(y.size()));
    const double w = (nbins > 0) ? (xMax - xMin) / nbins : 1.0;
    for(size_t i = 0; i < y.size(); ++i) {
        const double xc = xMin + (i + 0.5) * w;
        pts.append({xc, y[i]});
    }
    item -> ReceiveContents(pts);
    item -> SetTitle(title);
    item -> SetStats(stats);

    s.scene -> addItem(item);
    s.items.push_back(item);
    RelayoutSection(s);
}

void TrackingResultPanel::AddHisto2D(const std::string &title,
                                     const std::vector<std::string> &stats,
                                     int nx, double xMin, double xMax,
                                     int ny, double yMin, double yMax,
                                     const std::vector<double> &z)
{
    if(m_sections.empty()) return;
    Section &s = m_sections.back();

    HistoItem2D *item = new HistoItem2D();
    item -> SetData(nx, xMin, xMax, ny, yMin, yMax, z);
    item -> SetTitle(title);
    item -> SetStats(stats);

    s.scene -> addItem(item);
    s.items.push_back(item);
    RelayoutSection(s);
}

void TrackingResultPanel::RelayoutSection(Section &s)
{
    int w = s.view->viewport()->width() - 4;
    if(w < 50) w = 50;

    const int n = static_cast<int>(s.items.size());
    s.scene -> setSceneRect(0, 0, w, n * m_rowH);

    for(int i = 0; i < n; ++i) {
        const QRectF f(0, i * m_rowH, w, m_rowH);
        if(auto *it1 = dynamic_cast<HistoItem*>(s.items[i]))
            it1 -> SetBoundingRect(f);
        else if(auto *it2 = dynamic_cast<HistoItem2D*>(s.items[i]))
            it2 -> SetBoundingRect(f);
    }
}

void TrackingResultPanel::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    for(Section &s : m_sections)
        RelayoutSection(s);
}

};
