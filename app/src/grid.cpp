#include "grid.h"
#include <cmath>
#include <algorithm>

float Grid::pt_to_pixels(float pt) const {
    return pt * (dpi_scale_ / 72.0f) * 96.0f;
}

void Grid::set_spacing_pt(float spacing_pt) {
    spacing_pt_ = std::max(1.0f, spacing_pt);
}

vec2 Grid::world_to_screen(const vec2& world) const {
    float spacing_px = pt_to_pixels(spacing_pt_);
    vec2 offset = {(canvas_center_x_), (canvas_center_y_)};
    return {(world.x + pan_offset_.x) * zoom_level_ * spacing_px + offset.x,
            (-world.y + pan_offset_.y) * zoom_level_ * spacing_px + offset.y};
}

vec2 Grid::screen_to_world(const vec2& screen) const {
    float spacing_px = pt_to_pixels(spacing_pt_);
    vec2 offset = {(canvas_center_x_), (canvas_center_y_)};
    float world_x = (screen.x - offset.x) / (zoom_level_ * spacing_px) - pan_offset_.x;
    float world_y = -((screen.y - offset.y) / (zoom_level_ * spacing_px) - pan_offset_.y);
    return {world_x, world_y};
}

vec2 Grid::snap_to_grid(const vec2& world) const {
    float spacing_px = pt_to_pixels(spacing_pt_);
    float snapped_x = std::round(world.x) * spacing_px;
    float snapped_y = std::round(world.y) * spacing_px;
    return {snapped_x / spacing_px, snapped_y / spacing_px};
}

void Grid::set_viewport(const ImVec2& canvas_pos, const ImVec2& canvas_size) {
    canvas_pos_ = canvas_pos;
    canvas_size_ = canvas_size;
    canvas_center_x_ = canvas_pos.x + canvas_size.x * 0.5f;
    canvas_center_y_ = canvas_pos.y + canvas_size.y * 0.5f;
}

vec2 Grid::mouse_to_world_snapped() const {
    ImVec2 mouse_pos = ImGui::GetMousePos();
    return snap_to_grid(screen_to_world({mouse_pos.x, mouse_pos.y}));
}

void Grid::render() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2& canvas_pos = canvas_pos_;
    const ImVec2& canvas_size = canvas_size_;

    float spacing_px = pt_to_pixels(spacing_pt_) * zoom_level_;
    if (spacing_px < 5.0f) return;

    vec2 top_left = screen_to_world({canvas_pos.x, canvas_pos.y});
    vec2 bottom_right = screen_to_world({canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y});

    int start_x = (int)std::floor(top_left.x);
    int end_x = (int)std::ceil(bottom_right.x);
    int start_y = (int)std::floor(bottom_right.y);
    int end_y = (int)std::ceil(top_left.y);

    ImU32 grid_color = IM_COL32(60, 60, 60, 255);
    ImU32 axis_color = IM_COL32(100, 100, 100, 255);

    for (int x = start_x; x <= end_x; ++x) {
        vec2 world_pos = {(float)x, 0.0f};
        vec2 screen_top = world_to_screen({(float)x, (float)end_y});
        vec2 screen_bottom = world_to_screen({(float)x, (float)start_y});
        ImU32 color = (x == 0) ? axis_color : grid_color;
        draw_list->AddLine(
            ImVec2(screen_top.x, screen_top.y),
            ImVec2(screen_bottom.x, screen_bottom.y),
            color, 1.0f
        );
    }

    for (int y = start_y; y <= end_y; ++y) {
        vec2 screen_left = world_to_screen({(float)start_x, (float)y});
        vec2 screen_right = world_to_screen({(float)end_x, (float)y});
        ImU32 color = (y == 0) ? axis_color : grid_color;
        draw_list->AddLine(
            ImVec2(screen_left.x, screen_left.y),
            ImVec2(screen_right.x, screen_right.y),
            color, 1.0f
        );
    }
}

void Grid::handle_input() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = io.MouseDelta;
            float spacing_px = pt_to_pixels(spacing_pt_) * zoom_level_;
            pan_offset_.x += delta.x / spacing_px;
            pan_offset_.y -= delta.y / spacing_px;
        }

        if (io.MouseWheel != 0.0f) {
            float zoom_factor = 1.0f + io.MouseWheel * 0.1f;
            zoom_level_ = std::clamp(zoom_level_ * zoom_factor, 0.1f, 10.0f);
        }
    }
}
