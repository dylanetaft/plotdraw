#pragma once

#include "tools/tool.h"
#include <vector>
#include <memory>
#include <string>

class Toolbox {
public:
    Toolbox();

    // grid and polygon are handed to the tool's on_attach, for one-time wiring such as
    // subscribing to Polygon. They are not retained here.
    void add_tool(const std::string& name, std::unique_ptr<Tool> tool,
                  Grid& grid, Polygon& polygon);
    void set_active_tool(int index);
    int get_active_tool_index() const { return active_tool_; }
    Tool* get_active_tool();

    void render_tool_buttons();
    void render_properties(Grid& grid, Polygon& polygon);

private:
    struct ToolEntry {
        std::string name;
        std::unique_ptr<Tool> tool;
    };

    std::vector<ToolEntry> tools_;
    int active_tool_ = -1;
};
