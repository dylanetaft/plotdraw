#pragma once

class Polygon;

// Notified after a Polygon's geometry changes.
//
// Exists because ImGui is immediate mode: anything derived from the ring -- the OpenSCAD export
// string above all -- would otherwise be rebuilt inside the draw call, once per frame, whether or
// not the shape moved. Observers regenerate on mutation instead.
//
// Selection changes deliberately do NOT fire this. Nothing derived depends on which points are
// highlighted, and a box drag would otherwise notify on every frame it swept.
class PolygonObserver {
public:
    virtual ~PolygonObserver() = default;

    virtual void on_polygon_changed(const Polygon& polygon) = 0;
};
