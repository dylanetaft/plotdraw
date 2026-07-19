#pragma once

#include "tool.h"

class LoadTool : public Tool {
public:
    LoadTool();

    void on_property_changed(const std::string& id, Grid& grid, Polygon& polygon) override;
    void on_action(const std::string& id, Grid& grid, Polygon& polygon) override;
    void on_paste(const std::string& text, Grid& grid, Polygon& polygon) override;

private:
    void load_from_text(const std::string& text, Polygon& polygon);
};
