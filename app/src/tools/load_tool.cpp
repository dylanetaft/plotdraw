#include "load_tool.h"
#include "../utils/clipboard.h"

LoadTool::LoadTool() {
    properties_.addProperty("input", "Paste Vertex Data", PropertyType::String, "");
    add_action("paste", "Paste from Clipboard");
    set_tooltip(
        "Press Paste from Clipboard to load an OpenSCAD polygon() call, replacing the current "
        "shape. The browser will ask permission to read the clipboard the first time.\n\n"
        "You can also type or paste into the box directly.\n\n"
        "Points load in the order given rather than being re-ordered, so an outline stays the "
        "outline you pasted.");
}

void LoadTool::load_from_text(const std::string& text, Polygon& polygon) {
    // set_points, not add_point: pasted data is already an outline, and running
    // cheapest-edge insertion over it would silently reorder it.
    polygon.set_points(Polygon::parse_openscad_string(text));
}

void LoadTool::on_property_changed(const std::string& id, Grid& grid, Polygon& polygon) {
    if (id == "input") {
        load_from_text(properties_.getProperty("input"), polygon);
    }
}

void LoadTool::on_action(const std::string& id, Grid& grid, Polygon& polygon) {
    if (id == "paste") {
        // Only starts the read. On the web it resolves asynchronously, so the text arrives later
        // through on_paste rather than here.
        clipboard_request_paste();
    }
}

void LoadTool::on_paste(const std::string& text, Grid& grid, Polygon& polygon) {
    // Mirrored into the box as well as parsed, so the user can see and correct what arrived
    // instead of the shape changing with nothing to show for it.
    Property* prop = properties_.getPropertyPtr("input");
    if (prop) prop->value = text;
    load_from_text(text, polygon);
}
