// Interaction tests for the canvas gesture logic -- the press/drag/release state machine in
// SelectTool, and PlotTool's shift delegation to it.
//
// These mirror the interaction table in docs/PLAN.md. ImGui input is stubbed (tests/shim/imgui.h),
// so this exercises the decision logic, not real event delivery or rendering.
#include "tools/select_tool.h"
#include "tools/plot_tool.h"
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool cond, const std::string& what) {
    printf("%s  %s\n", cond ? "PASS" : "FAIL", what.c_str());
    if (!cond) failures++;
}

// A square, given CCW; set_points renormalizes it to CW, so the stored order is
// (0,4) (4,4) (4,0) (0,0). Tests address vertices by position, not index, so the winding
// normalization stays an implementation detail here.
static const std::vector<vec2> SQUARE = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};

static bool selected_at(const Polygon& p, vec2 world) {
    int i = p.find_point_at(world, 0.3f);
    return i >= 0 && p.is_selected(i);
}

static bool exists_at(const Polygon& p, vec2 world) {
    return p.find_point_at(world, 0.3f) >= 0;
}

// Drives a tool through a full gesture. World coordinates are converted to mouse position with
// the same 10x mapping as grid_stubs.cpp, so the 4px drag threshold behaves as it does in the app.
class Gesture {
public:
    Gesture(Tool& tool, Grid& grid, Polygon& poly) : tool_(tool), grid_(grid), poly_(poly) {}

    Gesture& shift(bool on) {
        imgui_test_io.KeyShift = on;
        return *this;
    }

    Gesture& press_at(vec2 world) {
        imgui_test_mouse = ImVec2(world.x * 10.0f, world.y * 10.0f);
        tool_.on_mouse_press(grid_.snap_to_grid(world), world, grid_, poly_);
        return *this;
    }

    Gesture& move_px(float dx, float dy) {
        imgui_test_mouse.x += dx;
        imgui_test_mouse.y += dy;
        vec2 w = at_mouse();
        tool_.on_mouse_drag(grid_.snap_to_grid(w), w, grid_, poly_);
        return *this;
    }

    Gesture& release() {
        vec2 w = at_mouse();
        tool_.on_mouse_release(grid_.snap_to_grid(w), w, grid_, poly_);
        return *this;
    }

private:
    vec2 at_mouse() const { return {imgui_test_mouse.x / 10.0f, imgui_test_mouse.y / 10.0f}; }

    Tool& tool_;
    Grid& grid_;
    Polygon& poly_;
};

int main() {
    Grid g;

    // --- Shift+click toggling -------------------------------------------------------------

    // The reported bug: the press pre-selects so a shift-drag has something visible to move, and
    // a release-time toggle that reads current state undoes it. Shift+click could only deselect.
    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        check(selected_at(p, {0, 4}), "shift+click on an unselected point selects it");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();   // on
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();   // off
        check(!selected_at(p, {0, 4}), "a second shift+click deselects it again");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        Gesture(t, g, p).shift(true).press_at({4, 0}).release();
        check(selected_at(p, {0, 4}) && selected_at(p, {4, 0}),
              "shift+click accumulates a multi-point selection");
        check(p.get_selection().size() == 2, "and selects nothing else");
    }

    // --- Plain click ----------------------------------------------------------------------

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        Gesture(t, g, p).shift(false).press_at({4, 0}).release();
        check(p.get_selection().size() == 1 && selected_at(p, {4, 0}),
              "plain click replaces the selection with just that point");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        Gesture(t, g, p).shift(false).press_at({2, 2}).release();   // empty space
        check(p.get_selection().empty(), "plain click on empty space clears the selection");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        Gesture(t, g, p).shift(true).press_at({2, 2}).release();    // empty space
        check(selected_at(p, {0, 4}), "shift+click on empty space preserves the selection");
    }

    // --- Movement threshold ---------------------------------------------------------------

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        // 20px right = 2 world units, well past the 4px threshold.
        Gesture(t, g, p).shift(true).press_at({0, 4}).move_px(20, 0).release();
        check(selected_at(p, {2, 4}), "shift+drag selects the point and moves it, not toggling it off");
        check(!exists_at(p, {0, 4}), "and it left its original position");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        // 2px is under the threshold, so this is still a click.
        Gesture(t, g, p).shift(true).press_at({0, 4}).move_px(2, 0).release();
        check(selected_at(p, {0, 4}), "a sub-threshold wobble still counts as a click");
        check(exists_at(p, {0, 4}), "and does not move the point");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(false).press_at({0, 4}).move_px(20, 0).release();
        check(exists_at(p, {2, 4}) && p.get_points().size() == 4,
              "plain drag moves a single point without changing the ring size");
    }

    // --- Box select -----------------------------------------------------------------------

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        // From (-1,-1) to (0.5,0.5): encloses (0,0) only.
        Gesture(t, g, p).shift(false).press_at({-1, -1}).move_px(15, 15).release();
        check(p.get_selection().size() == 1 && selected_at(p, {0, 0}),
              "box select picks up only the enclosed point");
    }

    {
        Polygon p; p.set_points(SQUARE);
        SelectTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();          // pre-select
        Gesture(t, g, p).shift(true).press_at({-1, -1}).move_px(15, 15).release();
        check(p.get_selection().size() == 2, "shift+box adds to the selection");
        Gesture(t, g, p).shift(false).press_at({-1, -1}).move_px(15, 15).release();
        check(p.get_selection().size() == 1, "plain box replaces it");
    }

    // --- PlotTool shift delegation --------------------------------------------------------

    {
        Polygon p; p.set_points(SQUARE);
        PlotTool t;
        Gesture(t, g, p).shift(true).press_at({0, 4}).release();
        check(p.get_points().size() == 4, "plot+shift on a point does not delete it");
        check(selected_at(p, {0, 4}), "plot+shift selects it instead");
    }

    {
        Polygon p; p.set_points(SQUARE);
        PlotTool t;
        Gesture(t, g, p).shift(false).press_at({0, 4}).release();
        check(p.get_points().size() == 3, "plot without shift still deletes an existing point");
    }

    {
        Polygon p; p.set_points(SQUARE);
        PlotTool t;
        Gesture(t, g, p).shift(false).press_at({2, 8}).release();
        check(p.get_points().size() == 5 && exists_at(p, {2, 8}),
              "plot without shift still adds a new point");
    }

    printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
