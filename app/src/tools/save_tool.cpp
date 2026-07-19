#include "save_tool.h"
#include "../utils/clipboard.h"

SaveTool::SaveTool() {
    properties_.addProperty("output", "OpenSCAD Output", PropertyType::ReadOnly, "");
    add_action("copy", "Copy to Clipboard");
    set_tooltip(
        "The current shape as an OpenSCAD polygon() call. Press Copy to Clipboard, or select the "
        "text and press Ctrl+C.\n\n"
        "Right-click and the browser's Edit menu will not see this text -- the box is drawn on a "
        "canvas, so there is no selectable text there for the browser to act on.\n\n"
        "It regenerates whenever the shape changes, so it is always current.");
}

SaveTool::~SaveTool() {
    if (polygon_) polygon_->remove_observer(this);
}

void SaveTool::on_attach(Grid& grid, Polygon& polygon) {
    polygon_ = &polygon;
    polygon.add_observer(this);

    // Generate once up front, or the box sits empty until the first edit.
    update_output(polygon);
}

void SaveTool::on_polygon_changed(const Polygon& polygon) {
    update_output(polygon);
}

void SaveTool::on_action(const std::string& id, Grid& grid, Polygon& polygon) {
    if (id == "copy") {
        // Regenerated rather than read back from the property, so a copy can never hand out a
        // stale string if the observer wiring is ever broken.
        clipboard_set_text(polygon.to_openscad_string());
    }
}

void SaveTool::update_output(const Polygon& polygon) {
    // Written through the raw pointer rather than setProperty, which rejects every write to a
    // ReadOnly property. ReadOnly means the *user* cannot type here, not that the program cannot
    // fill it in -- routing this through setProperty is exactly why the box was always blank.
    Property* prop = properties_.getPropertyPtr("output");
    if (prop) prop->value = polygon.to_openscad_string();
}
