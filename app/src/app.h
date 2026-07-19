#pragma once

#include "grid.h"
#include "polygon.h"
#include "toolbox.h"
#include <memory>

class App {
public:
    App();
    ~App();

    void init();
    void update();
    void render();
    void shutdown();

private:
    Grid grid_;
    Polygon polygon_;
    Toolbox toolbox_;

    float splitter_pos_ = 300.0f;
    bool dragging_splitter_ = false;

    void render_toolbox_panel();
    void render_grid_panel();
    void render_points();
    void render_polygon();
    void render_vertex_labels();
};
