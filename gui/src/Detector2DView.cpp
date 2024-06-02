#include "Detector2DView.h"
#include "ColorBar.h"
#include "APVStripMapping.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <iostream>

Detector2DView::Detector2DView(QWidget* parent) : QWidget(parent),
    scene(new QGraphicsScene(this)), view(new QGraphicsView(scene))
{
    InitView();

    ReDistributePaintingArea();

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout -> addWidget(view);

    color_bar = new ColorBar();
    layout -> addWidget(color_bar);
}

void Detector2DView::InitView()
{
    auto & layers = apv_strip_mapping::Mapping::Instance() -> GetLayerMap();

    for(auto &l: layers)
    {
        int ChamberPerLayer = l.second.chambers_per_layer;
        int n_strips_x = l.second.nb_apvs_x * 128, n_strips_y = l.second.nb_apvs_y * 128;
        std::string readout_type = l.second.readout_type;

        for(int i=0; i<ChamberPerLayer; ++i) {
            Detector2DAddress addr(l.first, i);
            det[addr] = new Detector2DItem();
            if(readout_type.find("UV") != std::string::npos) {
                det[addr] -> SetStripAngle(160, 160);
                det[addr] -> SetReadoutType(readout_type);
            }
            det[addr] -> SetTitle("layer" + std::to_string(l.first) +
                    " gem" + std::to_string(i));
            det[addr] -> SetStripIndexRange(0, n_strips_x, 0, n_strips_y);
            scene -> addItem(det[addr]);
        }
    }
}

void Detector2DView::ReDistributePaintingArea()
{
    // need to minus 20 to allow space for vertical and horizontal scroll bars
    int sceneRectWidth = width() - 20 > 0 ? width() - 20 : 100;
    int sceneRectHeight = height() - 20 > 0 ? height() - 20 : 50;

    // need to give space to color bar
    sceneRectWidth -= 110;
    sceneRectHeight -= 50;

    QRectF f(0, 0, sceneRectWidth, sceneRectHeight);
    scene -> setSceneRect(f);

    // distribute painting area
    auto & layers = apv_strip_mapping::Mapping::Instance() -> GetLayerMap();
    int NLayer = layers.size();

    int layer_index = 0;
    for(auto &l: layers)
    {
        int ChamberPerLayer = l.second.chambers_per_layer;
        for(int i=0; i<ChamberPerLayer; i++) 
        {
            Detector2DAddress addr(l.first, i);
            QRectF f(layer_index*sceneRectWidth/NLayer, i*sceneRectHeight/ChamberPerLayer, 
                    sceneRectWidth/NLayer, sceneRectHeight/ChamberPerLayer);
            det[addr] -> setPos(f.x(), f.y());

            // maybe it's better not to change boundingRect
            // otherwise the ratio will be changed
            det[addr] -> SetBoundingRect(f);
        }
        layer_index++;
    }
}

void Detector2DView::resizeEvent([[maybe_unused]] QResizeEvent *event)
{
    ReDistributePaintingArea();
}

////////////////////////////////////////////////////////////////////////////////
// this function needs to be improved, there's a deep copy need to be removed

void Detector2DView::FillEvent(std::pair<std::vector<int>, std::vector<int>> online_hits[][MaxChamberPerLayer])
{
    auto &layers = apv_strip_mapping::Mapping::Instance() -> GetLayerMap();

    // layer id vector
    auto &layerID = apv_strip_mapping::Mapping::Instance() -> GetLayerIDVec();

    // this is a same routine used in viewer
    // a helper to get layer_id position in vector
    // (due to the true layer_id may not start from 0, so layerID vector may not have 0)
    auto get_vector_index = [&](const int &layer_id) -> int
    {
        for(size_t i=0; i<layerID.size();++i)
        {
            if(layer_id == layerID[i])
                return i;
        }
        return -1;
    };

    for(auto &l: layers)
    {
        int ChamberPerLayer = l.second.chambers_per_layer;

        for(int i=0; i<ChamberPerLayer; ++i)
        {
            Detector2DAddress addr(l.first, i);
            // layer id index in vector
            int vec_index_id = get_vector_index(l.first);

            std::vector<std::pair<int, float>> x_strips;
            std::vector<std::pair<int, float>> y_strips;

            int x_index=0, y_index=0;
            for(auto &strip: online_hits[vec_index_id][i].first) {
                if(strip > 0)
                    x_strips.emplace_back(x_index, strip);
                x_index++;
            }
            for(auto &strip: online_hits[vec_index_id][i].second) {
                if(strip > 0)
                    y_strips.emplace_back(y_index, strip);
                y_index++;
            }

            det[addr] -> ReceiveContents(x_strips, y_strips);
            det[addr] -> update();
        }
    }
}
