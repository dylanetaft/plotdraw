# PlotDraw - OpenSCAD Vertex Array Visual Editor

## Overview
A visual tool for creating 2D polygon vertex arrays for OpenSCAD. Built with C++, imgui, SDL3, Emscripten, and CMake.

## Features
- Grid-based point placement with configurable spacing
- Concave and convex outlines; placement order does not matter
- Point dragging and multi-select
- Pan and zoom grid view
- Export to OpenSCAD polygon() format (read-only text box, regenerated on change)
- Import by pasting OpenSCAD text into the Load tool
- DPI-independent units (points)

## Architecture

### Project Structure
```
plotdraw/
├── CMakeLists.txt              # Superbuild
├── cmake/
│   └── imgui_CMakeLists.txt    # Build file patched into the imgui checkout
├── docs/
│   └── PLAN.md                 # This file
├── tests/                      # Native test build, separate from the superbuild
│   ├── CMakeLists.txt
│   ├── shim/imgui.h            # Stub <imgui.h>; scriptable input state
│   ├── grid_stubs.cpp          # Grid coordinate helpers
│   ├── test_polygon.cpp
│   ├── test_observer.cpp
│   ├── test_labels.cpp
│   └── test_gestures.cpp
├── app/
│   ├── CMakeLists.txt          # Main app
│   ├── src/
│   │   ├── main.cpp            # Entry point
│   │   ├── app.h/cpp           # Main app class, UI layout
│   │   ├── grid.h/cpp          # Grid rendering, pan/zoom
│   │   ├── polygon.h/cpp       # Point management, vertex ring
│   │   ├── polygon_observer.h  # Change-notification interface
│   │   ├── property.h/cpp      # Generic property system
│   │   ├── toolbox.h/cpp       # Toolbox panel
│   │   ├── tools/
│   │   │   ├── tool.h          # Tool interface
│   │   │   ├── plot_tool.h/cpp
│   │   │   ├── select_tool.h/cpp
│   │   │   ├── grid_tool.h/cpp
│   │   │   ├── save_tool.h/cpp
│   │   │   └── load_tool.h/cpp
│   └── emscripten/
│       └── shell.html
```

## UI Layout
```
┌─────────────────────────────────────────┐
│ [Tool Buttons]                          │
├──────────────┬──────────────────────────┤
│              │                          │
│  Properties  │      Grid View           │
│   Editor     │   (pan + zoom)           │
│              │                          │
│              │                          │
└──────────────┴──────────────────────────┘
```

## Property System

### Design
Generic property system for tool configuration:

```cpp
enum class PropertyType {
    String,
    Float,
    Int,
    Bool,
    ReadOnly
};

struct Property {
    std::string id;
    std::string name;
    PropertyType type;
    std::string value;
    float min_val = -FLT_MAX;
    float max_val = FLT_MAX;
    std::string error_msg;
};

class PropertySet {
    std::map<std::string, Property> properties;
    
    void addProperty(const std::string& id, const std::string& name, 
                     PropertyType type, const std::string& default_value);
    
    std::string getProperty(const std::string& id) const;
    bool setProperty(const std::string& id, const std::string& value);
    
    float getFloat(const std::string& id) const;
    int getInt(const std::string& id) const;
    bool getBool(const std::string& id) const;
};
```

### Benefits
- Generic UI rendering for any tool
- Easy to add new properties
- Validation with error messages
- Type-safe access for tools
- Serializable (future JSON support)

## Tool System

### Interface
```cpp
class Tool {
protected:
    PropertySet properties_;

public:
    // One-time wiring at registration, for anything needing the documents rather than a gesture
    // -- SaveTool subscribes to the Polygon here.
    virtual void on_attach(Grid&, Polygon&) {}

    // A gesture is press -> zero or more drags -> release.
    virtual void on_mouse_press(vec2 world_snapped, vec2 world_raw, Grid&, Polygon&) {}
    virtual void on_mouse_drag(vec2 world_snapped, vec2 world_raw, Grid&, Polygon&) {}
    virtual void on_mouse_release(vec2 world_snapped, vec2 world_raw, Grid&, Polygon&) {}

    // Drawn over the polygon, for in-progress gesture feedback.
    virtual void render_overlay(const Grid&, const Polygon&) const {}

    PropertySet& get_properties() { return properties_; }
    virtual void on_property_changed(const std::string& id, Grid&, Polygon&) {}

    // Instructions, drawn above this tool's properties. Set from the derived constructor.
    const std::string& get_tooltip() const { return tooltip_; }
};
```

Every tool sets a tooltip in its constructor, and `Toolbox::render_properties` draws it first,
wrapped, above a separator. It is the only in-app explanation of what a tool does — nothing else
tells the user that Shift extends a gesture, or that placement order does not matter.

`on_activate` was removed. Nothing implemented it, and `Toolbox::set_active_tool` never called it,
so it was a hook that could not fire. `on_attach` covers the real need.

Both world positions are supplied because they serve different needs: snapped for anything landing
on the grid (placing, moving), raw for anything continuous (a rubber band snapped to the grid would
jump in whole grid steps).

`App::render_grid_panel` drives the cycle from the canvas `InvisibleButton` — `IsItemActivated` →
press, `IsItemActive` → drag, `IsItemDeactivated` → release. The gesture state is latched *before*
`Grid::handle_input` runs, since that call reads `IsItemHovered` and needs the canvas button to
still be the current item.

The property-only tools (Save, Load) take no canvas input at all. Save keeps itself current by
observing the polygon; Load acts when its text box is edited.

### Interaction Model

The gesture is decided by **what is under the cursor at press time** and **whether the mouse moved**
before release. The move threshold is 4 screen pixels, so it is a constant physical distance
regardless of zoom.

| Press target | Modifier | Released without moving | Moved past threshold |
|---|---|---|---|
| a point | Shift | toggle that point | drag entire selection |
| a point | none, Select | select only it | drag it alone |
| a point | none, Plot | delete it | delete it (no drag) |
| empty | Shift | keep selection (no-op) | additive box select |
| empty | none, Select | clear selection | box select, replacing |
| empty | none, Plot | add a point | add a point |

Shift makes Plot behave as Select for every row, so plotting and adjusting do not require switching
tools. The two tools differ only in the unmodified-press rows.

A press on a point **pre-selects it**, so a shift-drag has something visible to move. That means
release-time toggling must compare against the selection state captured *before* the press
(`was_selected_at_press_`) — reading live state sees the press's own change and toggles the wrong
way, which makes shift+click deselect-only. Covered by `tests/test_gestures.cpp`.

Shift is latched at press time, not sampled per frame — otherwise releasing the key mid-drag would
switch modes partway through the gesture. A Shift-drag beginning on an *unselected* point selects it
first, so the gesture never moves something the user cannot see under the cursor.

### Tools

#### Plot Tool
- Press on empty → add a point (on press, not release, to keep placement immediate)
- Press on an existing point → remove it
- Shift held at press → forwards the whole gesture to an embedded `SelectTool`
- Properties: none (tooltip only)

#### Select Tool
Owns the selection gesture for the app; `PlotTool` composes one rather than duplicating the
hit-testing and box logic. Behavior is the table above.
- Properties: none (tooltip only)

Plot and Select both used to carry a ReadOnly count property. Neither ever updated: they wrote via
`setProperty`, which rejects every write to a ReadOnly property, so both displayed their `"0"`
default forever — in a 100px-tall multiline box, for a single number. Removed rather than repaired;
the counts were not worth the panel space.

#### Grid Tool
- Configure grid spacing
- Toggle vertex index labels
- Properties: Grid Spacing (pt) with min/max validation; Show Vertex Index (default on)

The label flag lives on `Grid`, not on `GridTool` — `App::render_points` is what reads it and
already holds a `Grid&`. Same split as spacing: the tool owns the widget, `Grid` owns the state.
The two defaults must be kept in agreement by hand, since `on_property_changed` fires only on a
user toggle and nothing syncs them at startup.

#### Save Tool
- Format the ring as an OpenSCAD polygon() call (no hull; the ring is already the outline)
- Properties: ReadOnly text area showing the array
- Implements `PolygonObserver`; subscribes in `on_attach` and regenerates only when the geometry
  changes. ImGui is immediate mode, so building the string during rendering would rebuild it every
  frame whether or not the shape moved.
- Writes the result through `getPropertyPtr(...)->value`, not `setProperty`, which rejects every
  write to a ReadOnly property. `ReadOnly` means the *user* cannot type there, not that the program
  cannot fill it in — routing it through `setProperty` is why the box was blank for so long.

#### Load Tool
- Parse vertex data from text
- Properties: Text area for pasting data
- `set_points`, not `add_point`: pasted data is already an outline, and cheapest-edge insertion
  would silently reorder it.

## Grid System

### Coordinate System
- World coordinates: 0,0 at center
- Screen coordinates: transformed by pan/zoom
- Grid spacing in points (1pt = 1/72 inch)

### Pan/Zoom
- Middle mouse drag → pan
- Scroll wheel → zoom
- Transform: screen = (world + pan) * zoom

### Snapping
- Round mouse position to nearest grid intersection
- Threshold-based hit testing for points

## Point Management

### Data Structure
```cpp
std::vector<vec2> points;  // Ordered ring: consecutive entries are edges, last wraps to first
```

The polygon is a stored ring, not a point cloud reduced to a hull on demand. This is what makes
concave shapes representable — a hull silently discards every interior vertex.

### Operations
- Place: Snap to grid, insert at the cheapest edge (below)
- Delete: Click on point → remove, joining its two neighbours
- Drag: Update position, snap to grid — ring position is *not* recomputed
- Box select: `select_in_rect` normalizes its corners itself, because the world frame is y-up
  while the screen is y-down — a box dragged toward the bottom-right arrives with a *decreasing*
  world y, and tools should not have to reason about that.

### Dragging
A drag is applied against a snapshot taken at `begin_drag`, never against the previous frame.
Applying a per-frame delta to current positions compounds: sub-grid deltas snap away to nothing
(so the selection never moves at all) and larger ones accumulate error.

The offset is snapped **once** and applied uniformly to every selected point. Snapping each point
after moving it would let the selection deform, since points can round toward different neighbours.

### Vertex Ordering: Cheapest-Edge Insertion
A point cloud does not determine a concave polygon — there are exponentially many simple
polygons through N points, and picking the shortest is Euclidean TSP (NP-hard). So order comes
from an insertion rule rather than a global solve.

A new point `P` splits the existing edge `(A,B)` minimising `|AP| + |PB| − |AB|`. This preserves
every existing adjacency; only the insertion position is chosen. The user never has to click in
outline order.

Edge-based, not nearest-vertex: the two nearest vertices are often non-adjacent (the tips of a
U), and splicing between non-adjacent vertices is not a defined ring operation.

Because insertion decides ring position, that order is not visible from the dots alone. The Grid
tool's **Show Vertex Index** draws each vertex's ring index next to it, so the resulting order can
be read directly off the canvas. Labels are suppressed when on-screen grid spacing drops below one
text line height, where they would overlap into a smear.

Each label is pushed along the **outward angle bisector** of the vertex's two edges
(`vertex_label.h`), so it clears the shape's own outline instead of being struck through by it; a
label pinned at a fixed offset lands on an edge whenever the geometry happens to run that way. The
math is done in screen space, since `world_to_screen` negates y and applies pan *before* that
negation — transforming the ring up front avoids reasoning about the flip and keeps the gap a
constant pixel distance at any zoom. Labels also draw in a pass *after* `render_polygon`, since
otherwise the edges paint over them regardless of placement.

Outward is decided by the ring's signed area recomputed each frame, **not** by the documented
clockwise invariant. `normalize_winding` runs only on `add_point` and `set_points`, so a drag can
leave the ring wound the other way with nothing restoring it; reading the invariant would flip every
label to the inside of the shape after such a drag.

Known limitations, accepted deliberately:
- Dragging does not re-solve, so a vertex dragged across the shape keeps its old neighbours.
- Self-intersection is possible and is not detected.
- History-dependent: the same points added in a different order can give a different polygon.

### Winding
`points` is always **clockwise in world coordinates** (negative signed area) — an invariant on
the stored ring, not a fixup at export. Insertion preserves traversal direction, so orientation
is settled when the third point lands and only needs re-checking where the ring is rebuilt
wholesale, which is now only load. Reversal remaps the selection via `i → n-1-i`; it is never done
mid-drag, since SelectTool holds an index Polygon cannot see.

OpenSCAD's 2D `polygon()` accepts either winding — CW is chosen for deterministic output and to
match the `polyhedron()` convention, where it *is* required.

### Change Notification
`Polygon` publishes geometry changes to registered `PolygonObserver`s. This exists because ImGui is
immediate mode: derived data — the OpenSCAD export string above all — would otherwise be rebuilt
inside the draw call, once per frame, whether or not the shape moved.

Three rules make it correct rather than merely present:

- **Fire at public mutator boundaries only**, never from `normalize_winding` or `shift_selection`.
  `add_point` and `set_points` both call into those, so notifying there would emit two events for
  one logical edit.
- **`drag_selection` notifies only when a position actually changed.** It runs once per mouse-move
  frame, and a snapped drag resolves to the already-applied offset on most of them. Notifying
  unconditionally would put the per-frame regeneration back that the observer exists to remove.
- **Selection changes do not fire it.** Nothing derived depends on which points are highlighted,
  and a box drag would otherwise notify on every frame it swept.

The non-const `get_points()` overload was deleted as part of this: it handed out a mutable
reference to the ring, letting a caller reshape it without any mutator running and so without any
notification. No call site was using it to mutate.

Observers are not owned. `SaveTool` unsubscribes in its destructor, which is safe only because
`App` declares `grid_` and `polygon_` *before* `toolbox_` — members destruct in reverse, so tools
die while the polygon is still alive. Reordering those declarations would make it a use-after-free.

## Clipboard Handling

Save has a **Copy to Clipboard** button, Load a **Paste from Clipboard** button, both registered
through the Tool action API (below). Ctrl+C in the Save box also works; Ctrl+V does not, for
reasons covered under "Why paste needs a button".

No copy-on-click, though: auto-copying on every canvas click meant the app silently took
ownership of the system clipboard while drawing, which is why the first attempt was removed. Copy
stays explicit and user-initiated.

### SDL has no clipboard on this target

The load-bearing fact, because it is invisible from the app side. SDL3's Emscripten backend
implements no clipboard at all — `src/video/emscripten/` contains zero clipboard code, so
`SDL_VideoDevice::SetClipboardData` is never populated and `SDL_SetClipboardText` falls through
to storing the string in wasm linear memory (see the `if (_this->SetClipboardData)` branch in
`src/video/SDL_clipboard.c`). Only the cocoa, windows, x11 and wayland backends install that hook.

So on web SDL's clipboard is a self-consistent in-process buffer: copy and paste appear to work
*within* the app while never touching the browser. That is exactly why the breakage was hard to
spot. `imgui_impl_sdl3` points ImGui's platform hooks straight at those SDL functions, so
InputText's Ctrl+C inherited the problem.

Tracked upstream as libsdl-org/SDL#13785 (milestone 3.6.0). We build against 3.4.2 — there is no
version to upgrade to, so `app/src/utils/clipboard.cpp` implements it.

### How it works instead

`main.cpp` overrides `ImGuiPlatformIO::Platform_Set/GetClipboardTextFn` *after*
`ImGui_ImplSDL3_InitForOpenGL` installs its own. Note these are the `ImGuiPlatformIO` hooks; the
same-named `ImGuiIO` fields are the obsolete pair and setting those compiles but does nothing.

- **Copy** — `navigator.clipboard.writeText`, with a `document.execCommand('copy')` fallback for
  when the browser rejects the call as too far from a user gesture. We reach it from the main
  loop's `requestAnimationFrame` tick rather than from the DOM handler, so activation is not
  guaranteed; Chrome's transient activation window is 5s, which covers it in practice.
- **Paste** — `navigator.clipboard.readText()`, started by the Paste button. It resolves into
  `plotdraw_on_clipboard_read`, which only *stores* the text; `App::update()` drains it on a later
  frame and hands it to the active tool's `on_paste`. Fire-and-forget rather than `EM_ASYNC_JS`:
  `-sASYNCIFY` is enabled and could `await` the read directly, but the call originates in
  `on_action`, which runs mid-ImGui-frame, and suspending there while rAF keeps dispatching risks
  corrupting ImGui's frame state.

`-sEXPORTED_RUNTIME_METHODS=ccall` and `-sEXPORTED_FUNCTIONS=_main,_plotdraw_on_clipboard_read`
are required. `ccall` is not exported by default and its absence surfaces as a runtime
`Module.ccall is not a function`, not a link error. Naming any `EXPORTED_FUNCTIONS` means `_main`
must be listed too or the app never starts.

### Why paste needs a button

The first attempt listened for the browser's `paste` event, which needs no permission. It never
fired. SDL's keydown handler classifies every Ctrl+key as a nav key
(`src/video/emscripten/SDL_emscriptenevents.c`, `is_nav_key`), which skips the
`prevent_default = false` escape hatch below it, so the handler returns true and the browser calls
`preventDefault()` on the keydown — and *generating the `paste` event is the default action being
cancelled*. Copy was unaffected because it calls `writeText` directly and depends on no browser
event.

That diagnosis was never confirmed empirically; the pivot to buttons made it moot. If someone
later wants prompt-free paste back, confirming it is the first step — the fix would be
intercepting Ctrl+V before SDL sees it.

`readText()` prompts for permission. Chrome remembers the grant per-origin; **Firefox shows a
paste confirmation on every use and cannot persist it**. That is a property of the API, not a bug.

### Tool actions

`Tool` carries a `std::vector<Action>` alongside its `PropertySet`, registered with `add_action`
from a derived constructor and dispatched through `on_action(id, grid, polygon)`. A button is a
verb the user invokes, not a value they edit, so it is deliberately not a `PropertyType` —
`PropertySet` carries value parsing, range checks and error strings that mean nothing for a press.

A vector, not the map `PropertySet` uses: that map iterates in alphabetical id order, which is
fine for named fields but would order buttons arbitrarily. Actions render in registration order,
after the properties, so a button reads as acting on the box above it.

`on_paste(text, grid, polygon)` is separate from `on_action` because it is an asynchronous
completion carrying a payload rather than a press.

### Known limitation

Right-click → Copy and the browser's Edit → Copy/Paste menus do not act on these boxes. They are
pixels on a canvas, so there is no DOM text for the browser to select. Fixing that needs a real
`<textarea>` overlaid on the canvas and kept in sync with the ImGui layout, which was judged not
worth it. The two buttons are the supported paths, with Ctrl+C as a bonus in the Save box.

## DPI-Independent Units

### Grid Spacing
- Stored in points (1pt = 1/72 inch)
- Convert to pixels: `pixels = points * dpi_scale * (1.0f / 72.0f) * 96.0f`
- Query DPI: `SDL_GetDisplayContentScale()`
- imgui handles UI scaling automatically

## Emscripten Build

### Configuration
- Use `emscripten_set_main_loop()` for web
- Configure shell.html with canvas sizing
- Link flags: `-s USE_SDL=3`

## Build System

### Superbuild Structure
1. Root CMakeLists.txt:
   - ExternalProject_Add for imgui and for the app
   - Configure app with prefix path pointing at the dependency staging root

2. cmake/imgui_CMakeLists.txt:
   - Copied over the imgui checkout's own CMakeLists.txt via PATCH_COMMAND
   - Fetch imgui from GitHub (docking branch)
   - Build with SDL3 + OpenGL3 backends
   - Install to ${CMAKE_BINARY_DIR}/deps

3. app/CMakeLists.txt:
   - Find SDL3 (system or emscripten)
   - Find imgui from the deps staging root
   - Handle emscripten-specific flags

### Build Directory Layout
```
build/
├── imgui-prefix/   # ExternalProject scratch (checkout, stamps) for imgui
├── app-prefix/     # ExternalProject scratch for the app
├── deps/           # dependencies install here; app finds them here
└── dist/           # final app install (bin/plotdraw.{html,js,wasm})
```

Both `*-prefix` dirs are ExternalProject's default naming — do not override `PREFIX`, or the
layout stops matching this doc.

### Building
```
# Native
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Emscripten
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

make -C build -j32
```

`make` is incremental and picks up edits under `app/src` — the app ExternalProject sets
`BUILD_ALWAYS ON`, so there is no need to delete `build/app-prefix` between builds. (Without it,
ExternalProject stamps the build step and silently skips changed sources.) The `-j` jobserver
propagates into the inner build.

To force a full dependency rebuild: `rm -rf build/imgui-prefix build/deps`.

## Testing

```
cmake -B build-tests tests
make -C build-tests -j32
ctest --test-dir build-tests --output-on-failure
```

A standalone native CMake project, deliberately outside the superbuild: the app builds under the
Emscripten toolchain, and these are host binaries. Guarding them with `if(NOT EMSCRIPTEN)` inside
`app/CMakeLists.txt` would mean they never build, since the normal build *is* the emscripten one.

- `tests/shim/imgui.h` — stub `<imgui.h>`: the types the logic touches, plus settable
  `imgui_test_io` / `imgui_test_mouse` so tests can script input. Placed ahead of `app/src` on the
  include path.
- `tests/grid_stubs.cpp` — `Grid`'s coordinate helpers, on a flat 10x world→screen mapping so the
  4px drag threshold stays meaningful. The rest of `grid.cpp` is rendering and is not linked.
- `test_polygon` — ring construction, winding, selection tracking, export/parse, dragging.
- `test_gestures` — the interaction table above, driven through a chainable `Gesture` helper.
- `test_labels` — `outward_normal_at`'s sign convention, reflex corners, and degenerate rings.
  The orientation assertions were mutation-checked: inverting the sign fails four of them.
- `test_observer` — one event per logical edit, selection changes staying silent, and drags
  notifying only on frames that moved something. Mutation-checked twice: a stray `notify()` in
  `normalize_winding` fails four assertions, and dropping the drag no-op guard fails four more.

This covers decision logic only. Rendering and real event delivery are not exercised, so visual and
input behavior still needs checking in a browser.

## Implementation Phases

### Phase 1: Build System
- [ ] CMake superbuild
- [ ] imgui external project
- [ ] App CMakeLists

### Phase 2: Core Framework
- [ ] main.cpp with SDL3 + imgui init
- [ ] App layout with splitter
- [ ] DPI scaling

### Phase 3: Grid System
- [ ] Grid rendering
- [ ] Pan/zoom
- [ ] Coordinate transform
- [ ] Grid snapping

### Phase 4: Property System
- [ ] PropertySet class
- [ ] Generic UI rendering

### Phase 5: Point Management
- [ ] Point placement/deletion
- [ ] Point dragging
- [ ] Cheapest-edge insertion ordering

### Phase 6: Tool System
- [ ] Tool base class
- [ ] Select tool
- [ ] Grid tool
- [ ] Save tool
- [ ] Load tool

### Phase 7: Emscripten
- [ ] Web build configuration
- [ ] Test in browser
