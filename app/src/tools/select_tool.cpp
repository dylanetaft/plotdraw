#include "select_tool.h"
#include <imgui.h>
#include <cmath>

// How far the mouse must travel before a press is treated as a drag rather than a click.
// Screen pixels, so it stays a constant physical distance regardless of zoom.
static constexpr float kDragThresholdPx = 4.0f;

// World-space radius for hit-testing a point, matching the plot tool.
static constexpr float kHitRadius = 0.3f;

SelectTool::SelectTool() {
    set_tooltip(
        "Click a point to select it, then drag to move it. Movement snaps to the grid.\n\n"
        "Drag from empty space to rubber-band select everything inside the box.\n\n"
        "Hold Shift to add to the selection instead of replacing it, to toggle a point off, and "
        "to drag a multi-point selection as a group.");
}

void SelectTool::on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    shift_ = ImGui::GetIO().KeyShift;
    moved_ = false;
    press_world_snapped_ = world_snapped;
    press_world_raw_ = world_raw;
    cursor_world_raw_ = world_raw;
    press_screen_ = ImGui::GetMousePos();
    press_hit_index_ = polygon.find_point_at(world_snapped, kHitRadius);

    // Captured before the pre-selection below mutates it. on_mouse_release toggles against this
    // rather than against live state, which would otherwise see the press's own change.
    was_selected_at_press_ = press_hit_index_ >= 0 && polygon.is_selected(press_hit_index_);

    if (press_hit_index_ >= 0) {
        mode_ = Mode::DragPoints;

        // Without shift the press replaces the selection immediately, so the shape being
        // dragged matches what is highlighted.
        if (!shift_) {
            polygon.clear_selection();
            polygon.select_point(press_hit_index_);
        } else if (!was_selected_at_press_) {
            // Shift-dragging an unselected point would otherwise move things the user cannot
            // see under the cursor, so bring it in now. If this turns out to be a click rather
            // than a drag, that is also the correct "toggle on" outcome and release leaves it.
            polygon.select_point(press_hit_index_);
        }

        polygon.begin_drag();
    } else {
        mode_ = Mode::BoxSelect;
    }
}

void SelectTool::on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    if (mode_ == Mode::None) return;

    cursor_world_raw_ = world_raw;

    if (!moved_) {
        ImVec2 now = ImGui::GetMousePos();
        float dx = now.x - press_screen_.x;
        float dy = now.y - press_screen_.y;
        if (std::sqrt(dx * dx + dy * dy) < kDragThresholdPx) return;
        moved_ = true;
    }

    if (mode_ == Mode::DragPoints) {
        // Measured from the press, not the last frame -- Polygon applies it to its snapshot.
        polygon.drag_selection(world_raw - press_world_raw_, grid);
    }
}

void SelectTool::on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid& grid, Polygon& polygon) {
    if (mode_ == Mode::None) return;

    if (!moved_) {
        // A press that never travelled is a click. The press already applied the no-shift
        // selection, so only the shift-toggle case is left to resolve.
        if (mode_ == Mode::DragPoints && shift_) {
            // Toggle against the pre-press state, not the current one: the press may already
            // have selected this point, and reading that back would toggle the wrong way.
            if (was_selected_at_press_) {
                polygon.deselect_point(press_hit_index_);
            }
            // Otherwise the press already selected it, which is the "toggle on" outcome.
        } else if (mode_ == Mode::BoxSelect && !shift_) {
            polygon.clear_selection();
        }
    } else if (mode_ == Mode::BoxSelect) {
        polygon.select_in_rect(press_world_raw_, world_raw, shift_);
    }

    polygon.end_drag();
    mode_ = Mode::None;
    moved_ = false;
    press_hit_index_ = -1;
    was_selected_at_press_ = false;
}

void SelectTool::render_overlay(const Grid& grid, const Polygon& polygon) const {
    if (mode_ != Mode::BoxSelect || !moved_) return;

    vec2 a = grid.world_to_screen(press_world_raw_);
    vec2 b = grid.world_to_screen(cursor_world_raw_);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 min(std::min(a.x, b.x), std::min(a.y, b.y));
    ImVec2 max(std::max(a.x, b.x), std::max(a.y, b.y));

    draw_list->AddRectFilled(min, max, IM_COL32(120, 180, 255, 40));
    draw_list->AddRect(min, max, IM_COL32(120, 180, 255, 200));
}
