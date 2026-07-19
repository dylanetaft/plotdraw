#include "polygon.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <utility>

// Shoelace formula. Positive means counter-clockwise in the y-up world frame.
static float signed_area(const std::vector<vec2>& pts) {
    float area = 0.0f;
    int n = (int)pts.size();
    for (int i = 0; i < n; ++i) {
        const vec2& a = pts[i];
        const vec2& b = pts[(i + 1) % n];
        area += a.cross(b);
    }
    return area * 0.5f;
}

void Polygon::add_observer(PolygonObserver* observer) {
    if (observer) observers_.push_back(observer);
}

void Polygon::remove_observer(PolygonObserver* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
}

void Polygon::notify() {
    for (PolygonObserver* observer : observers_) {
        observer->on_polygon_changed(*this);
    }
}

void Polygon::clear() {
    points_.clear();
    selection_.clear();
    notify();
}

void Polygon::shift_selection(int from, int delta) {
    std::set<int> shifted;
    for (int idx : selection_) {
        shifted.insert(idx >= from ? idx + delta : idx);
    }
    selection_ = shifted;
}

void Polygon::normalize_winding() {
    if (points_.size() < 3 || signed_area(points_) <= 0.0f) return;

    std::reverse(points_.begin(), points_.end());

    // Reversal maps index i to n-1-i, so the selection can follow exactly.
    int n = (int)points_.size();
    std::set<int> remapped;
    for (int idx : selection_) {
        remapped.insert(n - 1 - idx);
    }
    selection_ = remapped;
}

void Polygon::add_point(const vec2& point) {
    // Below 3 points there is no ring to insert into yet, so just append.
    if (points_.size() < 3) {
        points_.push_back(point);
        normalize_winding();
        notify();
        return;
    }

    // Split the edge that costs the least to detour through the new point. This preserves
    // every existing adjacency -- only the insertion position is being chosen.
    int n = (int)points_.size();
    int best_edge = 0;
    float best_cost = FLT_MAX;
    for (int i = 0; i < n; ++i) {
        const vec2& a = points_[i];
        const vec2& b = points_[(i + 1) % n];
        float cost = (point - a).length() + (b - point).length() - (b - a).length();
        if (cost < best_cost) {
            best_cost = cost;
            best_edge = i;
        }
    }

    // Splicing into an edge keeps the traversal direction, so winding is still valid here.
    int at = best_edge + 1;
    points_.insert(points_.begin() + at, point);
    shift_selection(at, +1);
    notify();
}

void Polygon::remove_point(int index) {
    if (index >= 0 && index < (int)points_.size()) {
        points_.erase(points_.begin() + index);
        selection_.erase(index);
        shift_selection(index, -1);
        notify();
    }
}

void Polygon::set_points(std::vector<vec2> pts) {
    points_ = std::move(pts);
    selection_.clear();
    normalize_winding();
    notify();
}

void Polygon::move_point(int index, const vec2& new_pos) {
    if (index >= 0 && index < (int)points_.size()) {
        if (points_[index] == new_pos) return;
        points_[index] = new_pos;
        notify();
    }
}

int Polygon::find_point_at(const vec2& world_pos, float threshold) const {
    for (int i = 0; i < (int)points_.size(); ++i) {
        vec2 diff = points_[i] - world_pos;
        if (diff.length() < threshold) {
            return i;
        }
    }
    return -1;
}

void Polygon::select_point(int index) {
    if (index >= 0 && index < (int)points_.size()) {
        selection_.insert(index);
    }
}

void Polygon::deselect_point(int index) {
    selection_.erase(index);
}

void Polygon::clear_selection() {
    selection_.clear();
}

void Polygon::select_all() {
    selection_.clear();
    for (int i = 0; i < (int)points_.size(); ++i) {
        selection_.insert(i);
    }
}

bool Polygon::is_selected(int index) const {
    return selection_.find(index) != selection_.end();
}

void Polygon::select_in_rect(const vec2& a, const vec2& b, bool additive) {
    if (!additive) selection_.clear();

    // Normalize here rather than in the caller: the world frame is y-up while the screen is
    // y-down, so a box dragged toward the bottom-right has a *decreasing* world y. Tools
    // should not have to reason about that.
    float min_x = std::min(a.x, b.x), max_x = std::max(a.x, b.x);
    float min_y = std::min(a.y, b.y), max_y = std::max(a.y, b.y);

    for (int i = 0; i < (int)points_.size(); ++i) {
        const vec2& p = points_[i];
        if (p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y) {
            selection_.insert(i);
        }
    }
}

void Polygon::begin_drag() {
    drag_snapshot_ = points_;
}

void Polygon::drag_selection(const vec2& total_delta, const Grid& grid) {
    // A drag that never began has no reference positions; ignore it rather than moving points
    // by a delta measured from nothing.
    if (drag_snapshot_.size() != points_.size()) return;

    // Snap the offset once and apply it uniformly. Snapping each point after moving it would
    // let the selection deform, since points can round toward different neighbours.
    vec2 offset = grid.snap_to_grid(total_delta);

    // Tracked so the notification only fires on frames that actually moved something. This runs
    // once per mouse-move frame, and a snapped drag spends most of them resolving to the offset
    // it already applied -- notifying regardless would put the per-frame regeneration that
    // observers exist to avoid straight back into the drag.
    bool changed = false;

    for (int idx : selection_) {
        if (idx >= 0 && idx < (int)drag_snapshot_.size()) {
            vec2 moved = drag_snapshot_[idx] + offset;
            if (points_[idx] != moved) {
                points_[idx] = moved;
                changed = true;
            }
        }
    }

    if (changed) notify();
}

void Polygon::end_drag() {
    drag_snapshot_.clear();
}

std::string Polygon::to_openscad_string() const {
    // points_ is already the outline, wound CW -- nothing to compute here.
    std::stringstream ss;
    ss << "polygon(points=[";
    for (int i = 0; i < (int)points_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "[" << points_[i].x << "," << points_[i].y << "]";
    }
    ss << "]);";
    return ss.str();
}

std::vector<vec2> Polygon::parse_openscad_string(const std::string& str) {
    std::vector<vec2> result;

    size_t pos = str.find("points=[");
    if (pos == std::string::npos) return result;

    pos += 8;  // past "points=[", now inside the outer list

    // Find the bracket closing the outer list. A plain find("]") would stop at the end of
    // the first coordinate pair, so match by depth.
    size_t end = std::string::npos;
    int depth = 1;
    for (size_t i = pos; i < str.size(); ++i) {
        if (str[i] == '[') ++depth;
        else if (str[i] == ']' && --depth == 0) { end = i; break; }
    }
    if (end == std::string::npos) return result;

    std::string points_str = str.substr(pos, end - pos);

    std::stringstream ss(points_str);
    char ch;
    while (ss >> ch) {
        if (ch == '[') {
            float x, y;
            ss >> x >> ch >> y;
            result.push_back({x, y});
        }
    }

    return result;
}
