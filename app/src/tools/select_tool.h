#pragma once

#include "tool.h"

// Owns the selection gesture for the whole app. PlotTool composes one of these and forwards to
// it while shift is held, so the hit-testing and box logic live in exactly one place.
class SelectTool : public Tool {
public:
    SelectTool();

    void on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) override;
    void render_overlay(const Grid& grid, const Polygon& polygon) const override;

private:
    // What the press landed on decides the gesture; whether the mouse then moved decides
    // between the click meaning and the drag meaning.
    enum class Mode { None, DragPoints, BoxSelect };

    Mode mode_ = Mode::None;
    bool moved_ = false;
    bool shift_ = false;

    vec2 press_world_snapped_;
    vec2 press_world_raw_;
    vec2 cursor_world_raw_;
    ImVec2 press_screen_;
    int press_hit_index_ = -1;

    // Whether press_hit_index_ was selected *before* the press touched it. The press pre-selects
    // so a shift-drag has something visible to move, which would otherwise corrupt a release-time
    // toggle that reads current state.
    bool was_selected_at_press_ = false;
};
