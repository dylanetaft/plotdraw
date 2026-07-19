#pragma once

#include "../types.h"
#include "../property.h"
#include "../grid.h"
#include "../polygon.h"
#include <string>
#include <utility>
#include <vector>

// A verb the user can invoke, as opposed to a Property, which is a value they edit. Drawn as a
// button. Deliberately not a PropertyType: PropertySet carries value parsing, range checks and
// error strings that mean nothing for a press, and setProperty would have to special-case it.
struct Action {
    std::string id;
    std::string label;
};

class Tool {
public:
    virtual ~Tool() = default;

    // Called once when the tool is registered with the Toolbox, for wiring that needs the
    // documents themselves rather than a gesture -- subscribing to Polygon, for instance.
    // Fires at startup, not on tool selection.
    virtual void on_attach(Grid& grid, Polygon& polygon) {}

    // A gesture is press -> zero or more drags -> release. Both world positions are supplied:
    // snapped for anything that lands on the grid (placing, moving), raw for anything
    // continuous (a rubber band snapped to the grid would jump in whole grid steps).
    virtual void on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {}
    virtual void on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {}
    virtual void on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {}

    // Drawn over the polygon each frame, for in-progress gesture feedback. Lets a tool own its
    // own visuals instead of App having to know which tools draw what.
    virtual void render_overlay(const Grid& grid, const Polygon& polygon) const {}

    virtual void on_deactivate() {}

    PropertySet& get_properties() { return properties_; }
    const PropertySet& get_properties() const { return properties_; }

    const std::vector<Action>& get_actions() const { return actions_; }

    virtual void on_property_changed(const std::string& id, Grid& grid, Polygon& polygon) {}

    // A button registered via add_action was pressed.
    virtual void on_action(const std::string& id, Grid& grid, Polygon& polygon) {}

    // Text arrived from the clipboard. Separate from on_action because it is an asynchronous
    // completion carrying a payload -- on the web the read resolves a frame or more after the
    // press that started it -- rather than the press itself.
    virtual void on_paste(const std::string& text, Grid& grid, Polygon& polygon) {}

    // Instructions for this tool, drawn above its properties. Empty means no tooltip block.
    const std::string& get_tooltip() const { return tooltip_; }

protected:
    // Set from the derived constructor. Protected rather than public: a tool's instructions
    // describe its own behaviour, so nothing outside it should be rewriting them.
    void set_tooltip(std::string text) { tooltip_ = std::move(text); }

    // Called from the derived constructor, same as addProperty. A vector rather than the map
    // PropertySet uses: that map iterates in alphabetical id order, which is fine for named
    // fields but would order buttons arbitrarily. Actions render in the order registered.
    void add_action(std::string id, std::string label) {
        actions_.push_back({std::move(id), std::move(label)});
    }

    PropertySet properties_;
    std::vector<Action> actions_;
    std::string tooltip_;
};
