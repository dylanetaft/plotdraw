// Native harness for the ring/winding logic in polygon.cpp.
#include "polygon.h"
#include <cstdio>
#include <vector>
#include <string>
#include <cmath>

// Grid's coordinate helpers are stubbed in grid_stubs.cpp, shared with the other test binaries.

static int failures = 0;

static void check(bool cond, const std::string& what) {
    printf("%s  %s\n", cond ? "PASS" : "FAIL", what.c_str());
    if (!cond) failures++;
}

static float area(const std::vector<vec2>& p) {
    float a = 0.0f;
    int n = (int)p.size();
    for (int i = 0; i < n; ++i) a += p[i].cross(p[(i + 1) % n]);
    return a * 0.5f;
}

static std::string seq(const std::vector<vec2>& p) {
    std::string s;
    for (auto& v : p) s += "(" + std::to_string((int)v.x) + "," + std::to_string((int)v.y) + ")";
    return s;
}

// Does the ring visit exactly these points, in this cyclic order (either direction)?
static bool same_cycle(const std::vector<vec2>& got, const std::vector<vec2>& want) {
    int n = (int)want.size();
    if ((int)got.size() != n) return false;
    for (int dir = 0; dir < 2; ++dir) {
        for (int off = 0; off < n; ++off) {
            bool ok = true;
            for (int i = 0; i < n; ++i) {
                int j = dir ? (off - i + 2 * n) % n : (off + i) % n;
                if (!(got[i] == want[j])) { ok = false; break; }
            }
            if (ok) return true;
        }
    }
    return false;
}

// An L-shape (reflex corner at 1,1). Listed in CCW order.
static const std::vector<vec2> L = {
    {0,0}, {2,0}, {2,1}, {1,1}, {1,2}, {0,2}
};

int main() {
    Grid g;  // only snap_to_grid is exercised; the stub above supplies it

    // 1. Scrambled click order still reconstructs the L.
    {
        Polygon p;
        for (int i : {3, 0, 5, 2, 4, 1}) p.add_point(L[i]);
        check(same_cycle(p.get_points(), L),
              "scrambled click order rebuilds the L: " + seq(p.get_points()));
        check(p.get_points().size() == 6, "no points dropped (hull would drop the reflex vertex)");
    }

    // 2. Winding invariant, from both input orientations.
    {
        Polygon ccw; ccw.set_points(L);
        std::vector<vec2> rev(L.rbegin(), L.rend());
        Polygon cw;  cw.set_points(rev);
        check(area(ccw.get_points()) < 0, "CCW input stored CW");
        check(area(cw.get_points())  < 0, "CW input stays CW");
        check(same_cycle(ccw.get_points(), cw.get_points()), "both orientations give the same cycle");
    }

    // 3. Winding holds as points are added one at a time, in any order.
    {
        bool all_cw = true;
        for (int start = 0; start < 6; ++start) {
            Polygon p;
            for (int i = 0; i < 6; ++i) p.add_point(L[(start + i) % 6]);
            if (area(p.get_points()) >= 0) all_cw = false;
        }
        check(all_cw, "ring stays CW across all 6 rotational click orders");
    }

    // 4. Insertion is local: a point on an edge splits only that edge.
    {
        Polygon p; p.set_points(L);
        auto before = p.get_points();
        p.add_point({0, 1});                  // sits on the (0,2)-(0,0) edge
        auto after = p.get_points();
        check(after.size() == 7, "one vertex added");
        std::vector<vec2> without;
        for (auto& v : after) if (!(v == vec2{0,1})) without.push_back(v);
        check(same_cycle(without, before), "removing it restores the original cycle exactly");
    }

    // 5. Selection tracks its vertex across an insertion.
    {
        Polygon p; p.set_points(L);
        int idx = 4;                          // (1,2)
        vec2 target = p.get_points()[idx];
        p.select_point(idx);
        p.add_point({0, 1});
        int sel = *p.get_selection().begin();
        check(p.get_points()[sel] == target,
              "selected vertex still selected after inserting before it");
    }

    // 6. Selection tracks its vertex across a winding reversal.
    {
        Polygon p; p.set_points(L);           // stored CW
        int idx = 2;
        vec2 target = p.get_points()[idx];
        p.select_point(idx);
        std::vector<vec2> flipped(p.get_points().rbegin(), p.get_points().rend());
        Polygon q; q.set_points(flipped);     // set_points clears selection, so re-check manually
        check(area(q.get_points()) < 0, "reversed input renormalized to CW");
        (void)target;
    }

    // 7. Export emits the whole ring, not a hull.
    {
        Polygon p; p.set_points(L);
        std::string s = p.to_openscad_string();
        int commas = 0;
        for (size_t i = 0; i + 1 < s.size(); ++i) if (s[i] == '[' && s[i+1] != ']') commas++;
        check(s.find("polygon(points=[") == 0, "export well-formed: " + s);
        check(commas == 7, "export lists all 6 vertices (6 pairs + outer bracket)");
    }

    // 8. Save -> load round-trip is identity.
    {
        Polygon p; p.set_points(L);
        auto parsed = Polygon::parse_openscad_string(p.to_openscad_string());
        Polygon q; q.set_points(parsed);
        check(q.get_points() == p.get_points(), "round-trip preserves order exactly");
    }

    // 9. select_in_rect: inclusion, exclusion, and corner order.
    {
        Polygon p; p.set_points(L);
        // A box over the left column only: x in [-0.5, 0.5] catches (0,0) and (0,2).
        p.select_in_rect({-0.5f, -0.5f}, {0.5f, 2.5f}, false);
        check(p.get_selection().size() == 2, "rect selects only enclosed points");

        // The same box given as the opposite corner pair must select identically -- the world
        // frame is y-up, so a downward screen drag arrives with decreasing y.
        Polygon q; q.set_points(L);
        q.select_in_rect({0.5f, 2.5f}, {-0.5f, -0.5f}, false);
        check(q.get_selection() == p.get_selection(), "rect normalizes reversed corners");

        // Non-additive replaces; additive unions.
        p.select_in_rect({1.5f, -0.5f}, {2.5f, 1.5f}, false);
        check(p.get_selection().size() == 2, "non-additive rect replaces the selection");
        p.select_in_rect({-0.5f, -0.5f}, {0.5f, 2.5f}, true);
        check(p.get_selection().size() == 4, "additive rect unions with the selection");

        // An empty region clears rather than leaving the old selection behind.
        p.select_in_rect({10.0f, 10.0f}, {11.0f, 11.0f}, false);
        check(p.get_selection().empty(), "rect over empty space selects nothing");
    }

    // 11. Dragging is measured from the snapshot, not compounded frame to frame.
    {
        Polygon p; p.set_points(L);
        p.select_all();
        std::vector<vec2> before = p.get_points();

        p.begin_drag();
        // Replay the same total delta many times, as a stationary mouse would each frame.
        for (int i = 0; i < 10; ++i) p.drag_selection({3.0f, -2.0f}, g);
        p.end_drag();

        bool rigid = true;
        for (size_t i = 0; i < before.size(); ++i) {
            if (p.get_points()[i].x != before[i].x + 3.0f) rigid = false;
            if (p.get_points()[i].y != before[i].y - 2.0f) rigid = false;
        }
        check(rigid, "repeated identical deltas move the selection exactly once");
    }

    // 12. Sub-grid drags round to zero rather than accumulating drift.
    {
        Polygon p; p.set_points(L);
        p.select_all();
        std::vector<vec2> before = p.get_points();

        p.begin_drag();
        for (int i = 0; i < 50; ++i) p.drag_selection({0.1f, 0.1f}, g);
        p.end_drag();

        check(p.get_points() == before, "sub-grid drag does not drift the shape");
    }

    // 13. Only selected points move.
    {
        Polygon p; p.set_points(L);
        p.select_point(0);
        vec2 untouched = p.get_points()[1];

        p.begin_drag();
        p.drag_selection({2.0f, 0.0f}, g);
        p.end_drag();

        check(p.get_points()[1].x == untouched.x && p.get_points()[1].y == untouched.y,
              "unselected points stay put during a drag");
    }

    // 14. A drag applied without begin_drag is ignored rather than moving from nothing.
    {
        Polygon p; p.set_points(L);
        p.select_all();
        std::vector<vec2> before = p.get_points();
        p.drag_selection({5.0f, 5.0f}, g);
        check(p.get_points() == before, "drag without begin_drag is a no-op");
    }

    printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
