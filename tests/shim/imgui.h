#pragma once
// Stub <imgui.h> for the native test build.
//
// The app's logic (polygon ring, tool gestures, properties) does not need real ImGui -- only a
// handful of types and three input queries. Supplying them here lets that logic be tested on the
// host, with no SDL, no GL context and no browser. Anything that actually draws is a no-op.
//
// Input state is exposed as mutable globals so tests can script a gesture: set imgui_test_io
// and imgui_test_mouse, then call the tool's press/drag/release directly.

struct ImVec2 {
    float x = 0, y = 0;
    ImVec2() = default;
    ImVec2(float a, float b) : x(a), y(b) {}
};

using ImU32 = unsigned int;

#define IM_COL32(r, g, b, a) \
    ((ImU32)(((ImU32)(a) << 24) | ((ImU32)(b) << 16) | ((ImU32)(g) << 8) | (ImU32)(r)))

struct ImGuiIO {
    bool KeyShift = false;
};

// Drawing is irrelevant to the logic under test; these exist only so render_overlay links.
struct ImDrawList {
    void AddRect(const ImVec2&, const ImVec2&, ImU32) {}
    void AddRectFilled(const ImVec2&, const ImVec2&, ImU32) {}
};

// Written by tests to drive the code under test. Named distinctly (rather than g_io/g_mouse) so
// it is obvious at the call site that this is test scaffolding, not real ImGui state.
inline ImGuiIO imgui_test_io;
inline ImVec2 imgui_test_mouse;

namespace ImGui {

inline ImGuiIO& GetIO() { return imgui_test_io; }
inline ImVec2 GetMousePos() { return imgui_test_mouse; }

inline ImDrawList* GetWindowDrawList() {
    static ImDrawList sink;
    return &sink;
}

}  // namespace ImGui
