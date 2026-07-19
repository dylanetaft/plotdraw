// Tests for Polygon's change notification -- the mechanism that lets derived data (the OpenSCAD
// export string) regenerate on mutation instead of every frame inside the immediate-mode draw.
#include "polygon.h"
#include "polygon_observer.h"
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool cond, const std::string& what) {
    printf("%s  %s\n", cond ? "PASS" : "FAIL", what.c_str());
    if (!cond) failures++;
}

// Records how often it was told, and by whom.
struct CountingObserver : public PolygonObserver {
    int calls = 0;
    const Polygon* last = nullptr;

    void on_polygon_changed(const Polygon& polygon) override {
        ++calls;
        last = &polygon;
    }
};

// A closed triangle, enough to have an interior and to exercise normalize_winding.
static std::vector<vec2> triangle() {
    return {{0, 0}, {4, 0}, {0, 4}};
}

int main() {
    // --- One event per logical edit -----------------------------------------------------------

    {
        Polygon poly;
        CountingObserver obs;
        poly.add_observer(&obs);

        poly.add_point({0, 0});
        check(obs.calls == 1, "add_point notifies exactly once");

        poly.add_point({4, 0});
        poly.add_point({0, 4});
        check(obs.calls == 3, "each add_point notifies, including the ones that close the ring");

        poly.remove_point(0);
        check(obs.calls == 4, "remove_point notifies exactly once");

        poly.clear();
        check(obs.calls == 5, "clear notifies exactly once");
    }

    {
        // set_points calls normalize_winding, which mutates both points_ and selection_. If
        // notification were fired from the private helper rather than the public boundary, one
        // logical edit would emit two events.
        Polygon poly;
        CountingObserver obs;
        poly.add_observer(&obs);

        // Counter-clockwise, so normalize_winding definitely reverses it.
        poly.set_points({{0, 0}, {4, 0}, {0, 4}});
        check(obs.calls == 1, "set_points notifies once, not twice, despite normalize_winding");
    }

    {
        Polygon poly;
        poly.set_points(triangle());

        CountingObserver obs;
        poly.add_observer(&obs);

        poly.move_point(0, {9, 9});
        check(obs.calls == 1, "move_point notifies exactly once");

        // Nothing derived changed, so nothing should be recomputed.
        poly.move_point(0, {9, 9});
        check(obs.calls == 1, "move_point to the same position does not notify");

        poly.move_point(99, {1, 1});
        check(obs.calls == 1, "move_point with an out-of-range index does not notify");
    }

    // --- Selection is deliberately not a geometry change ---------------------------------------

    {
        Polygon poly;
        poly.set_points(triangle());

        CountingObserver obs;
        poly.add_observer(&obs);

        poly.select_point(0);
        poly.select_all();
        poly.clear_selection();
        poly.select_in_rect({-1, -1}, {5, 5}, false);
        check(obs.calls == 0, "selection changes do not notify -- nothing derived depends on them");
    }

    // --- Drags notify only on frames that moved something --------------------------------------

    {
        // drag_selection runs once per mouse-move frame. A snapped drag resolves to the same
        // offset on most of them, and notifying anyway would restore the per-frame regeneration
        // the observer exists to eliminate.
        Grid grid;
        Polygon poly;
        poly.set_points(triangle());
        poly.select_all();

        CountingObserver obs;
        poly.add_observer(&obs);

        poly.begin_drag();

        poly.drag_selection({0, 0}, grid);
        check(obs.calls == 0, "a drag that moves nothing does not notify");

        poly.drag_selection({10, 0}, grid);
        check(obs.calls == 1, "a drag that moves a point notifies");

        // Same total delta as the previous frame: the points are already there.
        poly.drag_selection({10, 0}, grid);
        check(obs.calls == 1, "re-applying the same drag offset does not notify again");

        poly.end_drag();
        check(obs.calls == 1, "end_drag mutates nothing and does not notify");
    }

    {
        // A drag was never begun, so drag_selection bails before touching anything.
        Grid grid;
        Polygon poly;
        poly.set_points(triangle());
        poly.select_all();

        CountingObserver obs;
        poly.add_observer(&obs);

        poly.drag_selection({10, 0}, grid);
        check(obs.calls == 0, "drag_selection without begin_drag does not notify");
    }

    // --- Registration ---------------------------------------------------------------------------

    {
        Polygon poly;
        CountingObserver a, b;
        poly.add_observer(&a);
        poly.add_observer(&b);

        poly.add_point({0, 0});
        check(a.calls == 1 && b.calls == 1, "every registered observer is notified");

        poly.remove_observer(&a);
        poly.add_point({4, 0});
        check(a.calls == 1 && b.calls == 2, "remove_observer stops delivery to that observer only");
    }

    {
        Polygon poly;
        CountingObserver obs;
        poly.add_observer(&obs);
        poly.add_point({0, 0});
        check(obs.last == &poly, "the callback receives the polygon that actually changed");
    }

    {
        // Defensive: nothing should blow up on a null or unknown observer.
        Polygon poly;
        poly.add_observer(nullptr);
        CountingObserver stranger;
        poly.remove_observer(&stranger);
        poly.add_point({0, 0});
        check(true, "null add_observer and unknown remove_observer are survivable");
    }

    printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
