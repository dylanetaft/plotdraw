#pragma once

#include "types.h"
#include "grid.h"
#include "polygon_observer.h"
#include <vector>
#include <set>
#include <string>

// The points are an ordered ring, not a set: consecutive entries are polygon edges and the
// last wraps to the first. Two invariants hold whenever the ring has 3+ points:
//   - add_point splices a new vertex into the edge it is cheapest to insert into, so the
//     caller never has to place points in outline order.
//   - the ring is wound clockwise in world coordinates (negative signed area).
class Polygon {
public:
    void add_point(const vec2& point);
    void remove_point(int index);
    void move_point(int index, const vec2& new_pos);

    // Replaces the ring wholesale, preserving the given order (unlike add_point).
    void set_points(std::vector<vec2> pts);

    // Read-only by design. A mutable accessor would let a caller reshape the ring without any
    // of the mutators running, silently skipping observer notification.
    const std::vector<vec2>& get_points() const { return points_; }

    int find_point_at(const vec2& world_pos, float threshold = 0.5f) const;

    void select_point(int index);
    void deselect_point(int index);
    void clear_selection();
    void select_all();
    bool is_selected(int index) const;
    const std::set<int>& get_selection() const { return selection_; }

    // Selects every point inside the axis-aligned world rect spanned by a and b, in either
    // corner order. additive keeps the existing selection; otherwise it replaces it.
    void select_in_rect(const vec2& a, const vec2& b, bool additive);

    // A drag is applied against the positions the points held when it started, never against
    // the previous frame. Applying a per-frame delta to current positions compounds: sub-grid
    // deltas snap away to nothing and larger ones accumulate error.
    void begin_drag();
    void drag_selection(const vec2& total_delta, const Grid& grid);
    void end_drag();

    std::string to_openscad_string() const;
    static std::vector<vec2> parse_openscad_string(const std::string& str);

    void clear();

    // Observers are not owned. Each must outlive this Polygon or remove itself first.
    void add_observer(PolygonObserver* observer);
    void remove_observer(PolygonObserver* observer);

private:
    // Fired at public mutator boundaries only. Never from normalize_winding or shift_selection:
    // add_point and set_points both call into those, so notifying there would emit two events
    // for one logical edit.
    void notify();

    // Shifts every selected index >= from by delta, keeping the selection attached to the
    // same vertices across an insertion or removal.
    void shift_selection(int from, int delta);

    // Reverses the ring if it is wound CCW. Only valid where the ring is built or rebuilt
    // wholesale -- never during a drag, since SelectTool holds an index we cannot remap.
    void normalize_winding();

    std::vector<vec2> points_;
    std::set<int> selection_;

    std::vector<PolygonObserver*> observers_;

    // Positions of every point at begin_drag, parallel to points_. Empty when no drag is live.
    std::vector<vec2> drag_snapshot_;
};
