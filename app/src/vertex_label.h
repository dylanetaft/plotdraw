#pragma once

#include "types.h"
#include <vector>

// Direction to push a vertex's index label so it clears the shape's own edges.
//
// Returns a unit vector pointing away from the ring at vertex i. `ring` must already be in screen
// coordinates (y down): the world->screen mapping negates y and applies pan before that negation,
// so transforming the points first is both simpler and keeps the label gap a constant pixel
// distance at any zoom.
//
// Returns {0, 0} when the ring is too small to have an inside (fewer than 3 points); callers are
// expected to fall back to a fixed offset there.
vec2 outward_normal_at(const std::vector<vec2>& ring, int i);
