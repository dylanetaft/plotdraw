#pragma once

#include "tool.h"

class GridTool : public Tool {
public:
    GridTool();

    void on_property_changed(const std::string& id, Grid& grid, Polygon& polygon) override;
};
