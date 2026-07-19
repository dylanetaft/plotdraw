#pragma once

#include "tool.h"
#include "../polygon_observer.h"

// Shows the polygon as an OpenSCAD call. Observes the Polygon rather than regenerating in the
// draw call: ImGui is immediate mode, so building the string while rendering would rebuild it
// every frame whether or not the shape moved.
class SaveTool : public Tool, public PolygonObserver {
public:
    SaveTool();
    ~SaveTool() override;

    void on_attach(Grid& grid, Polygon& polygon) override;
    void on_polygon_changed(const Polygon& polygon) override;
    void on_action(const std::string& id, Grid& grid, Polygon& polygon) override;

private:
    // Not owned. Held only so the destructor can unsubscribe.
    Polygon* polygon_ = nullptr;

    void update_output(const Polygon& polygon);
};
