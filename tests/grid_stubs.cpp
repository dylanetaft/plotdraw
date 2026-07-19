// grid.cpp is not compiled into the tests -- it is all rendering and ImGui input handling. Only
// its coordinate helpers are reachable from the logic under test, so they are stubbed here.
//
// The mapping is a plain 10x scale: one world unit is 10 screen pixels. That keeps the drag
// threshold (4 screen px, select_tool.cpp) meaningful against world coordinates in tests, without
// dragging DPI scaling, pan and zoom into it.
#include "grid.h"
#include <cmath>

float Grid::pt_to_pixels(float pt) const { return pt; }

vec2 Grid::world_to_screen(const vec2& world) const {
    return {world.x * 10.0f, world.y * 10.0f};
}

vec2 Grid::screen_to_world(const vec2& screen) const {
    return {screen.x / 10.0f, screen.y / 10.0f};
}

// Mirrors the real implementation (grid.cpp), where the spacing factors cancel and the result is
// a round to integer world units. An identity stub would make the drag tests vacuous.
vec2 Grid::snap_to_grid(const vec2& world) const {
    return {std::round(world.x), std::round(world.y)};
}
