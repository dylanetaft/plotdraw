#pragma once

#include "tool.h"
#include "select_tool.h"

// Plots points on an unmodified press. Holding shift at press time hands the entire gesture to
// an embedded SelectTool, so plotting and adjusting do not require switching tools.
class PlotTool : public Tool {
public:
    PlotTool();

    void on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void render_overlay(const Grid& grid, const Polygon& polygon) const override;

private:
    SelectTool select_;

    // Latched at press. Sampling KeyShift mid-gesture would switch modes partway through if the
    // key were released before the mouse.
    bool selecting_ = false;
};
