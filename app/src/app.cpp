#include "app.h"
#include "vertex_label.h"
#include "tools/plot_tool.h"
#include "tools/select_tool.h"
#include "tools/grid_tool.h"
#include "tools/save_tool.h"
#include "tools/load_tool.h"
#include "utils/clipboard.h"
#include <imgui.h>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

// Screen radius of a vertex dot. Shared so label placement can clear it exactly.
static constexpr float kPointRadius = 5.0f;

App::App() {
}

App::~App() {
}

void App::init() {
    // grid_ and polygon_ reach each tool's on_attach, which is where SaveTool subscribes to the
    // polygon. Safe because app.h declares grid_ and polygon_ *before* toolbox_: members destruct
    // in reverse, so the tools unsubscribe while the polygon is still alive. Reordering those
    // declarations would turn the destructor into a use-after-free.
    toolbox_.add_tool("Plot", std::make_unique<PlotTool>(), grid_, polygon_);
    toolbox_.add_tool("Select", std::make_unique<SelectTool>(), grid_, polygon_);
    toolbox_.add_tool("Grid", std::make_unique<GridTool>(), grid_, polygon_);
    toolbox_.add_tool("Save", std::make_unique<SaveTool>(), grid_, polygon_);
    toolbox_.add_tool("Load", std::make_unique<LoadTool>(), grid_, polygon_);
    toolbox_.set_active_tool(0);
}

void App::update() {
    // A clipboard read started by a Paste press resolves asynchronously on the web, so it lands
    // here rather than in the button handler. Drained before render() so the tool sees the text
    // outside of any in-progress ImGui frame.
    std::string pasted;
    if (clipboard_take_paste(pasted)) {
        if (Tool* tool = toolbox_.get_active_tool()) {
            tool->on_paste(pasted, grid_, polygon_);
        }
    }
}

void App::render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    // This window is pinned to the display and must never scroll: the canvas draws in absolute
    // screen coordinates, so a scroll offset would slide the grid out from under the mouse
    // mapping. NoScrollWithMouse also keeps the wheel free for Grid::handle_input's zoom.
    ImGui::Begin("PlotDraw", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Content region, not GetWindowSize(): the latter is the outer size, so sizing children to it
    // overflows by WindowPadding and raises a scrollbar on a window that must never scroll.
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("ToolboxPanel", ImVec2(splitter_pos_, avail.y), true);
    render_toolbox_panel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::InvisibleButton("##splitter", ImVec2(5.0f, avail.y));
    if (ImGui::IsItemActive()) {
        dragging_splitter_ = true;
    } else if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (dragging_splitter_) {
        splitter_pos_ += io.MouseDelta.x;
        splitter_pos_ = std::max(150.0f, std::min(splitter_pos_, avail.x - 150.0f));
        if (!ImGui::IsMouseDown(0)) {
            dragging_splitter_ = false;
        }
    }

    ImGui::SameLine();

    ImGui::BeginChild("GridPanel", ImVec2(0, avail.y), true);
    render_grid_panel();
    ImGui::EndChild();

    ImGui::End();
}

void App::render_toolbox_panel() {
    toolbox_.render_tool_buttons();
    ImGui::Separator();
    toolbox_.render_properties(grid_, polygon_);
}

void App::render_grid_panel() {
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton("##grid_canvas", canvas_size);
    ImVec2 canvas_pos = ImGui::GetItemRectMin();

    // Latch the button's gesture state before anything else queries the current item --
    // Grid::handle_input calls IsItemHovered and must still see the canvas button.
    bool pressed = ImGui::IsItemActivated();
    bool held = ImGui::IsItemActive();
    bool released = ImGui::IsItemDeactivated();

    grid_.set_viewport(canvas_pos, canvas_size);
    grid_.render();
    grid_.handle_input();

    render_points();
    render_polygon();
    // After the polygon: labels are annotation and must not be painted over by an edge.
    render_vertex_labels();

    Tool* tool = toolbox_.get_active_tool();
    if (tool) {
        ImVec2 mouse = ImGui::GetMousePos();
        vec2 raw = grid_.screen_to_world({mouse.x, mouse.y});
        vec2 snapped = grid_.mouse_to_world_snapped();

        if (pressed) {
            tool->on_mouse_press(snapped, raw, grid_, polygon_);
        } else if (released) {
            tool->on_mouse_release(snapped, raw, grid_, polygon_);
        } else if (held) {
            tool->on_mouse_drag(snapped, raw, grid_, polygon_);
        }

        // After the polygon so in-progress feedback draws on top of it.
        tool->render_overlay(grid_, polygon_);
    }
}

void App::render_points() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)polygon_.get_points().size(); ++i) {
        const vec2& pt = polygon_.get_points()[i];
        vec2 screen = grid_.world_to_screen(pt);

        ImU32 color = polygon_.is_selected(i) ? IM_COL32(255, 200, 0, 255) : IM_COL32(0, 200, 255, 255);

        draw_list->AddCircleFilled(
            ImVec2(screen.x, screen.y),
            kPointRadius,
            color
        );
    }
}

void App::render_vertex_labels() {
    if (!grid_.get_show_vertex_indices()) return;

    const std::vector<vec2>& ring = polygon_.get_points();
    if (ring.empty()) return;

    // outward_normal_at works in screen space: the world->screen mapping negates y and applies pan
    // before that negation, so transforming up front avoids reasoning about the flip, and keeps the
    // label gap a constant pixel distance at any zoom.
    std::vector<vec2> screen_ring;
    screen_ring.reserve(ring.size());
    for (const vec2& pt : ring) screen_ring.push_back(grid_.world_to_screen(pt));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)screen_ring.size(); ++i) {
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        ImVec2 size = ImGui::CalcTextSize(label);

        const vec2& screen = screen_ring[i];
        vec2 dir = outward_normal_at(screen_ring, i);

        ImVec2 pos;
        if (dir.length_squared() == 0.0f) {
            // Under 3 points there is no enclosed area and so no outward direction; sit below the
            // dot, which is where labels lived before the ring could have an inside.
            pos = ImVec2(screen.x - size.x * 0.5f, screen.y + kPointRadius + 2.0f);
        } else {
            // Clear the dot and the label's own box: AddText anchors at the top-left, so project
            // the text half-extent onto the direction before recentring.
            float reach = kPointRadius + 3.0f +
                          std::fabs(dir.x) * size.x * 0.5f +
                          std::fabs(dir.y) * size.y * 0.5f;
            vec2 center = screen + dir * reach;
            pos = ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f);
        }

        // Grey so the label reads as annotation and does not compete with the dot colors.
        draw_list->AddText(pos, IM_COL32(180, 180, 180, 255), label);
    }
}

void App::render_polygon() {
    const std::vector<vec2>& ring = polygon_.get_points();
    if (ring.size() < 3) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)ring.size(); ++i) {
        int next = (i + 1) % ring.size();
        vec2 screen1 = grid_.world_to_screen(ring[i]);
        vec2 screen2 = grid_.world_to_screen(ring[next]);

        draw_list->AddLine(
            ImVec2(screen1.x, screen1.y),
            ImVec2(screen2.x, screen2.y),
            IM_COL32(0, 255, 100, 255),
            2.0f
        );
    }
}

void App::shutdown() {
}
