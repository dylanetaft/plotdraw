#pragma once

#include "types.h"
#include <imgui.h>

class Grid {
public:
    void set_spacing_pt(float spacing_pt);
    float get_spacing_pt() const { return spacing_pt_; }

    void set_pan(const vec2& pan) { pan_offset_ = pan; }
    vec2 get_pan() const { return pan_offset_; }

    void set_zoom(float zoom) { zoom_level_ = zoom; }
    float get_zoom() const { return zoom_level_; }

    // Owned here rather than by GridTool because App::render_points is what reads it, and it
    // already holds a Grid&. Same split as spacing: the tool owns the widget, Grid owns the
    // state. The default must match GridTool's property default, since on_property_changed
    // only fires on a user toggle and nothing syncs the two at startup.
    bool get_show_vertex_indices() const { return show_vertex_indices_; }
    void set_show_vertex_indices(bool show) { show_vertex_indices_ = show; }

    // On-screen distance between gridlines. Exposed so callers can back off when the view is
    // too dense to annotate; pt_to_pixels itself stays private.
    float get_spacing_px() const { return pt_to_pixels(spacing_pt_) * zoom_level_; }

    // Must be called each frame before render/handle_input/mouse_to_world_snapped.
    // Grid is the sole owner of the canvas origin; callers pass absolute ImGui
    // screen coordinates and never offset by canvas_pos themselves.
    void set_viewport(const ImVec2& canvas_pos, const ImVec2& canvas_size);

    vec2 world_to_screen(const vec2& world) const;
    vec2 screen_to_world(const vec2& screen) const;
    vec2 snap_to_grid(const vec2& world) const;
    vec2 mouse_to_world_snapped() const;

    void render();
    void handle_input();

    float get_dpi_scale() const { return dpi_scale_; }
    void set_dpi_scale(float scale) { dpi_scale_ = scale; }

private:
    float spacing_pt_ = 10.0f;
    vec2 pan_offset_ = {0.0f, 0.0f};
    float zoom_level_ = 1.0f;
    float dpi_scale_ = 1.0f;
    bool show_vertex_indices_ = true;
    ImVec2 canvas_pos_ = {0.0f, 0.0f};
    ImVec2 canvas_size_ = {0.0f, 0.0f};
    float canvas_center_x_ = 0.0f;
    float canvas_center_y_ = 0.0f;

    float pt_to_pixels(float pt) const;
};
