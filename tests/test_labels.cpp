// Tests for vertex index label placement -- the outward normal used to push a label clear of the
// polygon's own edges.
//
// All rings here are in SCREEN space (y down), which is what outward_normal_at expects.
#include "vertex_label.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool cond, const std::string& what) {
    printf("%s  %s\n", cond ? "PASS" : "FAIL", what.c_str());
    if (!cond) failures++;
}

static vec2 centroid(const std::vector<vec2>& ring) {
    vec2 sum;
    for (const vec2& p : ring) sum += p;
    return sum / (float)ring.size();
}

static bool is_unit(vec2 v) { return std::fabs(v.length() - 1.0f) < 1e-4f; }

// The square a clockwise-in-world ring maps to on screen, from the hand-worked sign derivation:
// world (0,4) (4,4) (4,0) (0,0) -> screen with y negated. Signed area is positive here.
static const std::vector<vec2> SQUARE = {{0, -4}, {4, -4}, {4, 0}, {0, 0}};

int main() {
    // --- Orientation: the assertion that catches an inverted sign --------------------------

    {
        vec2 c = centroid(SQUARE);
        bool all_out = true;
        for (int i = 0; i < 4; ++i) {
            vec2 dir = outward_normal_at(SQUARE, i);
            if (dir.dot(SQUARE[i] - c) <= 0.0f) all_out = false;
        }
        check(all_out, "every corner normal points away from the centroid");
    }

    {
        bool all_unit = true;
        for (int i = 0; i < 4; ++i) {
            if (!is_unit(outward_normal_at(SQUARE, i))) all_unit = false;
        }
        check(all_unit, "returned directions are unit length");
    }

    {
        // A drag can reverse the ring with nothing renormalizing it, so orientation must come from
        // the geometry rather than an assumed winding. Same corners, opposite order, same answers.
        std::vector<vec2> reversed(SQUARE.rbegin(), SQUARE.rend());
        // SQUARE[0] is the last element of `reversed`.
        vec2 a = outward_normal_at(SQUARE, 0);
        vec2 b = outward_normal_at(reversed, (int)reversed.size() - 1);
        check((a - b).length() < 1e-4f, "reversing the winding gives the same outward direction");
    }

    {
        vec2 dir = outward_normal_at(SQUARE, 1);   // corner (4,-4): +x and -y on screen
        check(dir.x > 0.0f && dir.y < 0.0f, "the corner normal bisects both of its edges");
    }

    // --- Reflex vertices ------------------------------------------------------------------

    {
        // An L: the notch corner at (4,-4) is reflex. Interior lies toward -x/+y from it, so an
        // outward direction must have a positive x or negative y component -- never both inward.
        std::vector<vec2> L = {{0, -8}, {4, -8}, {4, -4}, {8, -4}, {8, 0}, {0, 0}};
        vec2 dir = outward_normal_at(L, 2);
        check(is_unit(dir), "reflex corner still returns a unit vector");
        check(dir.x > 0.0f && dir.y < 0.0f, "reflex corner points out of the material, not into the notch");
    }

    // --- Degenerate cases -----------------------------------------------------------------

    {
        // A collinear point mid-edge: both adjacent normals coincide, so the bisector is just
        // that edge's normal -- pointing straight up out of the bottom edge (screen -y).
        std::vector<vec2> ring = {{0, -4}, {4, -4}, {8, -4}, {8, 0}, {0, 0}};
        vec2 dir = outward_normal_at(ring, 1);
        check(is_unit(dir) && std::fabs(dir.x) < 1e-4f && dir.y < 0.0f,
              "a collinear vertex returns its edge normal");
    }

    {
        // A 180-degree spike doubles back on itself: the two normals cancel and the bisector sums
        // to zero. Must fall back to an edge normal rather than stacking the label on the dot.
        std::vector<vec2> spike = {{0, 0}, {4, 0}, {0, 0.001f}, {4, 4}, {0, 4}};
        vec2 dir = outward_normal_at(spike, 1);
        check(is_unit(dir), "a 180-degree spike still returns a unit vector, not {0,0}");
    }

    {
        check(outward_normal_at({}, 0).length_squared() == 0.0f, "an empty ring returns {0,0}");
        check(outward_normal_at({{0, 0}}, 0).length_squared() == 0.0f, "a single point returns {0,0}");
        check(outward_normal_at({{0, 0}, {4, 0}}, 0).length_squared() == 0.0f,
              "two points enclose no area, so there is no outward direction");
    }

    {
        // Callers index by loop counter; an out-of-range index must not read past the ring.
        check(outward_normal_at(SQUARE, 9).length_squared() == 0.0f, "out-of-range index returns {0,0}");
        check(outward_normal_at(SQUARE, -1).length_squared() == 0.0f, "negative index returns {0,0}");
    }

    printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
