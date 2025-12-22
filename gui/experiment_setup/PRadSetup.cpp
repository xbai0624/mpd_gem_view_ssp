#include "PRadSetup.h"
#include "GEMDetector.h"
#include "GEMPlane.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <cmath>
#include <QTransform>
#include <QPointF>
#include <QColor>

#define PI 3.1415926

PRadSetup::PRadSetup(QWidget *parent, int layer_id) : QWidget(parent), layer_id(layer_id)
{
    InitSetup();
}

PRadSetup::~PRadSetup()
{
}

void PRadSetup::InitSetup()
{
    // detector geometry
    det_w = 1100.4, det_h = 1228.8, overlap_d = 44.0; // note: det_w is 2*single_detector_w, it doesn't consider overlap
    hole_diameter = 44.;
    const double x_spacer_pos[6] = {171, 347, 523, 729.8, 920.8, 1096.8};
    std::copy_n(x_spacer_pos, 6, horizontal_spacer_pos);
    const double y_spacer_pos[2] = {183.8, 366.2};
    std::copy_n(y_spacer_pos, 2, vertical_spacer_pos);

    scene = new QGraphicsScene(this);
    scene -> setBackgroundBrush(QColor("#ECECEC"));
    view = new QGraphicsView(this);
    view -> setFrameShape(QFrame::NoFrame);
    view -> setScene(scene);

    QVBoxLayout *v = new QVBoxLayout(this);
    v -> setContentsMargins(0, 0, 0, 0);
    v -> setSpacing(2);
    v -> addWidget(view);
    setLayout(v);

    // detector sensitive area
    auto sensi = drawDetectionArea();
    for(auto &i: sensi)
        scene -> addItem(i);

    // detector spacer
    auto spacers = drawSpacers();
    for(auto &i: spacers)
        scene -> addItem(i);

    // draw APVs
    auto apvs = drawAPVs();
    for(auto &i: apvs)
        scene -> addItem(i);

    // draw arrows
    auto arrow1 = drawArrow(0, -160, 0, 400, QString("PRad2-GEM%1%2").arg(2*layer_id+1).arg('X'), QColor("#ff0000"));
    for(auto &i: arrow1)
        scene -> addItem(i);
    auto arrow2 = drawArrow(-200, 0, 90, 1228, QString("PRad2-GEM%1%2").arg(2*layer_id+1).arg('Y'), QColor("#ff0000"));
    for(auto &i: arrow2)
        scene -> addItem(i);
    auto arrow3 = drawArrow(1050, 1250, 180, 400, QString("PRad2-GEM%1%2").arg(2*layer_id+2).arg('X'), QColor("#008000"));
    for(auto &i: arrow3)
        scene -> addItem(i);
    auto arrow4 = drawArrow(1260, 1250, 270, 1228, QString("PRad2-GEM%1%2").arg(2*layer_id+2).arg('Y'), QColor("#008000"));
    for(auto &i: arrow4)
        scene -> addItem(i);
}

std::vector<QGraphicsItem*> PRadSetup::drawDetectionArea()
{
    std::vector<QGraphicsItem*> res;

    // detector sensitive area
    QGraphicsRectItem *plane = new QGraphicsRectItem(0, 0, det_w - overlap_d, det_h);
    plane -> setPen(QPen(Qt::black, 8));
    plane -> setPos(0, 0);
    res.push_back(plane);

    // beam hole
    QGraphicsEllipseItem *beam_hole = new QGraphicsEllipseItem(0, 0, hole_diameter, hole_diameter);
    beam_hole -> setPen(QPen(Qt::red, 8));
    beam_hole -> setPos(det_w/2-hole_diameter/2 - overlap_d/2, det_h/2-hole_diameter/2);
    res.push_back(beam_hole);

    return res;
}

std::vector<QGraphicsItem*> PRadSetup::drawSpacers()
{
    std::vector<QGraphicsItem*> res;

    // detector spacer
    for(int i=0; i<6; i++) {
        // horizontal spacer on left chambmer
        QGraphicsLineItem *line = new QGraphicsLineItem(0, 0, det_w/2, 0);
        line -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line -> setPos(0, horizontal_spacer_pos[i]);
        res.push_back(line);

        // horizontal spacer on right chambmer
        QGraphicsLineItem *line1 = new QGraphicsLineItem(0, 0, det_w/2, 0);
        line1 -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line1 -> setPos(det_w/2 - hole_diameter, det_h - horizontal_spacer_pos[5-i]);
        res.push_back(line1);
    }
    for(int i=0; i<2; i++) {
        // vertical spacer on left chambmer
        QGraphicsLineItem *line = new QGraphicsLineItem(0, 0, 0, det_h);
        line -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line -> setPos(vertical_spacer_pos[i], 0);
        res.push_back(line);

        // vertical spacer on right chamber
        QGraphicsLineItem *line1 = new QGraphicsLineItem(0, 0, 0, det_h);
        line1 -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line1 -> setPos(det_w - hole_diameter - vertical_spacer_pos[i], 0);
        res.push_back(line1);
    }

    // detector vertical frame
    for(int i=0; i<1; i++) {
        // vertical frame on left chambmer
        QGraphicsLineItem *line = new QGraphicsLineItem(0, 0, 0, det_h);
        line -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line -> setPos(det_w/2, 0);
        res.push_back(line);
        // vertical frame on right chambmer
        QGraphicsLineItem *line1 = new QGraphicsLineItem(0, 0, 0, det_h);
        line1 -> setPen(QPen(Qt::lightGray, 6, Qt::DashLine));
        line1 -> setPos(det_w/2 - hole_diameter, 0);
        res.push_back(line1);
    }

    return res;
}

std::vector<QGraphicsItem*> PRadSetup::drawAPVs()
{
    std::vector<QGraphicsItem*> res;

    // apv
    double apv_w = 50, apv_h = 70;
    double apv_det_d = 20;
    int apv_font_size = 40;
    for(int i=0; i<6; i++) {
        // 6 slot on left chamber
        QGraphicsRectItem *apv = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv -> setBrush(Qt::red);
        apv -> setPen(QPen(Qt::black, 4));
        double x = i * apv_w, y = -apv_h - apv_det_d;
        apv -> setPos(x, y);
        res.push_back(apv);
        // pos index
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(i));
        label -> setPos(x, y-60);
        label -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label);

        // 6 slot on right chamber
        QGraphicsRectItem *apv1 = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv1 -> setBrush(Qt::darkGreen);
        apv1 -> setPen(QPen(Qt::black, 4));
        double x1 = det_w - overlap_d - (i+1) * apv_w, y1 = det_h + apv_h + 2.2*apv_det_d;
        apv1 -> setPos(x1, y1);
        res.push_back(apv1);
        // pos index
        QGraphicsTextItem *label1 = new QGraphicsTextItem(QString::number(i));
        label1 -> setPos(x1, y1-60);
        label1 -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label1);
    }

    for(int i=0; i<5; i++) {
        // 5 slot on left chamber
        QGraphicsRectItem *apv = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv -> setBrush(Qt::red);
        apv -> setPen(QPen(Qt::black, 4));
        double x = 6*apv_w + i*apv_w;
        double y = det_h + apv_det_d;
        apv -> setPos(x, y);
        res.push_back(apv);
        // pos index
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(6+i));
        label -> setPos(x, y+60);
        label -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label);

        // 5 slot on right chamber
        QGraphicsRectItem *apv1 = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv1 -> setBrush(Qt::darkGreen);
        apv1 -> setPen(QPen(Qt::black, 4));
        double x1 = det_w - overlap_d - 6*apv_w - (i+1)*apv_w;
        double y1 = -apv_h -2*apv_w;
        apv1 -> setPos(x1, y1);
        res.push_back(apv1);
        // pos index
        QGraphicsTextItem *label1 = new QGraphicsTextItem(QString::number(6+i));
        label1 -> setPos(x1, y1-60);
        label1 -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label1);
    }

    for(int i=0; i<1; i++) {
        // single slot on left chamber
        QGraphicsRectItem *apv = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv -> setBrush(Qt::red);
        apv -> setPen(QPen(Qt::black, 4));
        double x = (6+4)*apv_w + i*apv_w;
        double y = -apv_h - apv_det_d;
        apv -> setPos(x, y);
        res.push_back(apv);
        // pos index
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(11));
        label -> setPos(x-50, y);
        label -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label);

        // single slot on right chamber
        QGraphicsRectItem *apv1 = new QGraphicsRectItem(0, 0, apv_w, apv_h);
        apv1 -> setBrush(Qt::darkGreen);
        apv1 -> setPen(QPen(Qt::black, 4));
        double x1 = det_w - overlap_d - 10*apv_w - (i+1)*apv_w;
        double y1 = det_h + apv_h + 2.2*apv_det_d;
        apv1 -> setPos(x1, y1);
        res.push_back(apv1);
        // pos index
        QGraphicsTextItem *label1 = new QGraphicsTextItem(QString::number(11));
        label1 -> setPos(x1+50, y1);
        label1 -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label1);
    }

    for(int i=0; i<24; i++) {
        // two 12-slot on left chamber
        QGraphicsRectItem *apv = new QGraphicsRectItem(0, 0, apv_h, apv_w);
        apv -> setBrush(Qt::red);
        apv -> setPen(QPen(Qt::black, 4));
        double x = -apv_h - apv_det_d;
        double y = i*51.2;
        apv -> setPos(x, y);
        res.push_back(apv);
        // pos index
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(i));
        label -> setPos(x-50, y);
        label -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label);

        // two 12-slot on right chamber
        QGraphicsRectItem *apv1 = new QGraphicsRectItem(0, 0, apv_h, apv_w);
        apv1 -> setBrush(Qt::darkGreen);
        apv1 -> setPen(QPen(Qt::black, 4));
        double x1 = det_w - overlap_d + apv_det_d;
        double y1 = i*51.2;
        apv1 -> setPos(x1, y1);
        res.push_back(apv1);
        // pos index
        QGraphicsTextItem *label1 = new QGraphicsTextItem(QString::number(23-i));
        label1 -> setPos(x1+apv_h, y1);
        label1 -> setFont(QFont("Times New Roman", apv_font_size));
        res.push_back(label1);
    }
    return res;
}

std::vector<QGraphicsItem*> PRadSetup::drawArrow(double x_pos, double y_pos, double angle, double length, QString label, QColor c)
{
    std::vector<QGraphicsItem*> res;

    double h_shaft_length = length;
    QGraphicsLineItem *shaft = new QGraphicsLineItem(0, 0, h_shaft_length, 0);
    shaft -> setRotation(angle);
    shaft -> setPen(QPen(c, 6));
    shaft -> setPos(x_pos, y_pos);
    res.push_back(shaft);

    double slope_length = 10;
    QGraphicsLineItem *angle1 = new QGraphicsLineItem(0, 0, slope_length, slope_length);
    angle1 -> setRotation(angle);
    angle1 -> setPen(QPen(c, 6));
    double x_a1 = x_pos + h_shaft_length - slope_length/sqrt(2), y_a1 = y_pos-slope_length/sqrt(2)-4;
    auto _t = QTransform().translate(x_pos, y_pos).rotate(angle).translate(-x_pos, -y_pos).map(QPointF(x_a1, y_a1));
    angle1 -> setPos(_t.x(), _t.y());
    res.push_back(angle1);

    QGraphicsLineItem *angle2 = new QGraphicsLineItem(0, 0, slope_length, -slope_length);
    angle2 -> setRotation(angle);
    angle2 -> setPen(QPen(c, 6));
    double x_a2 = x_pos + h_shaft_length - slope_length/sqrt(2), y_a2 = y_pos+slope_length/sqrt(2)+4;
    _t = QTransform().translate(x_pos, y_pos).rotate(angle).translate(-x_pos, -y_pos).map(QPointF(x_a2, y_a2));
    angle2 -> setPos(_t.x(), _t.y());
    res.push_back(angle2);

    QGraphicsTextItem *arrow1_text = new QGraphicsTextItem(label);
    arrow1_text -> setFont(QFont("Times New Roman", 40));
    arrow1_text -> setDefaultTextColor(c);
    QFont f = arrow1_text -> font();
    f.setItalic(true);
    arrow1_text -> setFont(f);
    int len = arrow1_text -> boundingRect().width();
    double t_pos_x = x_pos + (h_shaft_length - len)/2, t_pos_y = y_pos - 50;
    _t = QTransform().translate(x_pos, y_pos).rotate(angle).translate(-x_pos, -y_pos).map(QPointF(t_pos_x, t_pos_y));
    arrow1_text -> setPos(_t.x(), _t.y());
    arrow1_text -> setRotation(angle);
    res.push_back(arrow1_text);

    return res;
}

void PRadSetup::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    view -> fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void PRadSetup::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    view -> fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

QSize PRadSetup::sizeHint() const
{
    return QSize(400, 400);
}

void PRadSetup::DrawEventHits2D(const QMap<int, QVector<QPointF>> &data)
{
    PassData(data);

    // draw current event
    for(auto &i: Hits2D)
        scene -> addItem(i);
}

void PRadSetup::PassData(const QMap<int, QVector<QPointF>> &data)
{
    // delete previous event
    for(auto &i: Hits2D) {
        scene -> removeItem(i);
        delete i;
    }
    Hits2D.clear();

    auto coord_transform = [&](const QPointF &hit, bool left_chamber) -> std::pair<double, double>
    {
        double x = hit.x(), y = hit.y();

        // for eel setup
        if(left_chamber) {
            x = (det_w/4 + x);
            y = det_h/2 + y;
        }
        else {
            x = (det_w - overlap_d) - (det_w/4 + x);
            y = det_h/2 - y;
        }
        return std::pair<double, double>(x, y); // for eel setup

        // for PRad2 setup
        if(left_chamber) {
            x = det_w/4 + x;
            y = det_h/2 + y;
        }
        else {
            x = det_w/4 - x;
            y = det_h/2 - y;
        }

        return std::pair<double, double>(x, y);
    };

    // specific to detector actual installation
    // <gem_detector_id, layer_id>
    std::unordered_map<int, int> det_id_to_layer_id = {{1, 1}, {2, 1}, {3, 2}, {4, 2}};
    // <gem_detector_id, left or right>
    std::unordered_map<int, bool> det_id_to_is_left = {{1, true}, {2, false}, {3, true}, {4, false}};

    for(auto it = data.begin(); it != data.end(); ++it)
    {
        int det_id = it.key();
        if( det_id_to_layer_id[det_id] != layer_id )
            continue;

        const QVector<QPointF> & hits = it.value();

        bool is_left = det_id_to_is_left[det_id];

        for(const QPointF &p: hits) {
            auto pts = coord_transform(p, is_left);
            //std::cout<<"detector id: "<<det_id<<"   ("<<pts.first<<", "<<pts.second<<")"<<std::endl;

            auto *h = new QGraphicsEllipseItem(0, 0, 40, 40);
            if(is_left) {
                h -> setBrush(Qt::red);
                h -> setPen(QPen(Qt::red, 8));
            } else {
                h -> setBrush(Qt::darkGreen);
                h -> setPen(QPen(Qt::darkGreen, 8));
            }
            h -> setPos(pts.first, pts.second);
            Hits2D.push_back(h);
        }
    }
}
