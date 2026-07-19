#include "plot_tool.h"
#include <imgui.h>

PlotTool::PlotTool() {
    set_tooltip(
        "Click empty space to add a point. Click an existing point to remove it.\n\n"
        "New points splice into whichever edge is cheapest to detour through, so you can place "
        "them in any order rather than tracing the outline.\n\n"
        "Hold Shift to select and drag points without leaving this tool.");
}

void PlotTool::on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    selecting_ = ImGui::GetIO().KeyShift;
    if (selecting_) {
        select_.on_mouse_press(world_snapped, world_raw, grid, polygon);
        return;
    }

    // Acting on press rather than release keeps placement feeling as immediate as it was.
    int existing_idx = polygon.find_point_at(world_snapped, 0.3f);
    if (existing_idx >= 0) {
        polygon.remove_point(existing_idx);
    } else {
        polygon.add_point(world_snapped);
    }
}

void PlotTool::on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    if (selecting_) select_.on_mouse_drag(world_snapped, world_raw, grid, polygon);
}

void PlotTool::on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    if (selecting_) {
        select_.on_mouse_release(world_snapped, world_raw, grid, polygon);
        selecting_ = false;
    }
}

void PlotTool::render_overlay(const Grid& grid, const Polygon& polygon) const {
    if (selecting_) select_.render_overlay(grid, polygon);
}
