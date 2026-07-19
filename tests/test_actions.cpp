// Tests for Tool's action mechanism -- the button path that Save's Copy and Load's Paste ride on.
// Actions are kept separate from properties because a button is a verb, not an editable value,
// and unlike PropertySet's map they must render in the order registered.
#include "tools/tool.h"
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool cond, const std::string& what) {
    printf("%s  %s\n", cond ? "PASS" : "FAIL", what.c_str());
    if (!cond) failures++;
}

// Registers actions in an order that is deliberately not alphabetical, so a map-backed
// implementation would reorder them and fail the ordering check below.
struct RecordingTool : public Tool {
    std::vector<std::string> fired;
    std::vector<std::string> pasted;

    RecordingTool() {
        add_action("zebra", "Zebra");
        add_action("apple", "Apple");
    }

    void on_action(const std::string& id, Grid& grid, Polygon& polygon) override {
        fired.push_back(id);
    }

    void on_paste(const std::string& text, Grid& grid, Polygon& polygon) override {
        pasted.push_back(text);
    }
};

int main() {
    Grid grid;
    Polygon polygon;

    // --- Registration ---------------------------------------------------------------------------

    {
        RecordingTool tool;
        const std::vector<Action>& actions = tool.get_actions();

        check(actions.size() == 2, "both registered actions are exposed");
        check(actions[0].id == "zebra" && actions[1].id == "apple",
              "actions keep registration order rather than sorting by id");
        check(actions[0].label == "Zebra", "label round-trips alongside the id");
    }

    // --- Dispatch -------------------------------------------------------------------------------

    {
        RecordingTool tool;
        tool.on_action("apple", grid, polygon);

        check(tool.fired.size() == 1 && tool.fired[0] == "apple",
              "on_action receives the id that was pressed");
    }

    {
        // A tool that registers actions but overrides nothing must not crash: the base on_action
        // and on_paste are no-op virtuals, which is what lets a tool opt out of either.
        struct BareTool : public Tool {
            BareTool() { add_action("noop", "No-op"); }
        } tool;

        tool.on_action("noop", grid, polygon);
        tool.on_paste("text", grid, polygon);
        check(true, "base on_action and on_paste are safe no-ops");
    }

    // --- Paste delivery -------------------------------------------------------------------------

    {
        RecordingTool tool;
        tool.on_paste("polygon([[0,0],[1,0]]);", grid, polygon);

        check(tool.pasted.size() == 1 && tool.pasted[0] == "polygon([[0,0],[1,0]]);",
              "on_paste delivers the clipboard text verbatim");
    }

    {
        // Tools with no actions are the common case -- Plot, Select, Grid all register none.
        struct PlainTool : public Tool {} tool;
        check(tool.get_actions().empty(), "a tool that registers nothing exposes no actions");
    }

    printf("\n%s\n", failures == 0 ? "all passed" : "FAILURES");
    return failures == 0 ? 0 : 1;
}
