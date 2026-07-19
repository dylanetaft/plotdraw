#include "grid_tool.h"

GridTool::GridTool() {
    properties_.addProperty("spacing", "Grid Spacing (pt)", PropertyType::Float, "10.0");
    auto* prop = properties_.getPropertyPtr("spacing");
    if (prop) {
        prop->min_val = 1.0f;
        prop->max_val = 100.0f;
    }

    // "true" exactly -- toolbox.cpp compares against that literal. Must match Grid's own
    // default, since nothing syncs them until the user toggles the box.
    properties_.addProperty("show_indices", "Show Vertex Index", PropertyType::Bool, "true");

    set_tooltip(
        "Sets the grid spacing and toggles the numeric index drawn beside each vertex.\n\n"
        "These bindings work under any tool: scroll to zoom, and drag with the middle mouse "
        "button to pan.");
}

void GridTool::on_property_changed(const std::string& id, Grid& grid, Polygon& polygon) {
    if (id == "spacing") {
        float spacing = properties_.getFloat("spacing");
        grid.set_spacing_pt(spacing);
    } else if (id == "show_indices") {
        grid.set_show_vertex_indices(properties_.getBool("show_indices"));
    }
}
