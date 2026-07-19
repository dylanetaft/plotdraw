#include "vertex_label.h"

namespace {

// Twice the signed area of the ring. Only the sign is used, so the factor of 1/2 is skipped.
//
// Computed fresh from the geometry on screen rather than read off Polygon's documented clockwise
// invariant: normalize_winding only runs on add_point and set_points, so a drag can leave the ring
// wound the other way with nothing restoring it. Trusting the invariant would flip every label to
// the inside of the shape after such a drag.
float signed_area_2x(const std::vector<vec2>& ring) {
    float sum = 0.0f;
    for (int i = 0; i < (int)ring.size(); ++i) {
        const vec2& a = ring[i];
        const vec2& b = ring[(i + 1) % ring.size()];
        sum += a.cross(b);
    }
    return sum;
}

}  // namespace

vec2 outward_normal_at(const std::vector<vec2>& ring, int i) {
    int n = (int)ring.size();
    // Fewer than 3 points encloses no area, so there is no "outside" to point at.
    if (n < 3 || i < 0 || i >= n) return {0.0f, 0.0f};

    // Add n before the modulo: C++ % on a negative left operand yields a negative result.
    const vec2& prev = ring[(i + n - 1) % n];
    const vec2& cur = ring[i];
    const vec2& next = ring[(i + 1) % n];

    vec2 n_in = perp((cur - prev).normalized());
    vec2 n_out = perp((next - cur).normalized());

    // The angle bisector of the two edge normals, which points away from the shape at reflex
    // corners as well as convex ones. Collinear edges are fine here -- the normals coincide and
    // the sum is just twice one of them.
    vec2 bisector = (n_in + n_out).normalized();

    // A 180-degree spike doubles back, making n_in == -n_out and the sum zero. normalized() returns
    // {0,0} rather than NaN, which is safe but would stack the label on top of the dot.
    if (bisector.length_squared() == 0.0f) bisector = n_out;

    // perp() rotates toward the interior for a positively-wound ring, so flip there. Worked through
    // on a clockwise-in-world square, which maps to screen area +16: the raw bisector at its
    // top-right corner points at the centroid.
    return signed_area_2x(ring) > 0.0f ? bisector * -1.0f : bisector;
}
