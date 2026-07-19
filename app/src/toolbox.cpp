#include "toolbox.h"
#include <imgui.h>
#include <string>

namespace {

// Lets an InputText grow its std::string instead of capping at a fixed buffer. ImGui calls this
// when the text no longer fits, handing back the buffer it should use.
int input_text_resize(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        std::string* str = static_cast<std::string*>(data->UserData);
        str->resize(data->BufTextLen);
        data->Buf = str->data();
    }
    return 0;
}

}  // namespace

Toolbox::Toolbox() {
}

void Toolbox::add_tool(const std::string& name, std::unique_ptr<Tool> tool,
                       Grid& grid, Polygon& polygon) {
    if (tool) tool->on_attach(grid, polygon);
    tools_.push_back({name, std::move(tool)});
}

void Toolbox::set_active_tool(int index) {
    if (index >= 0 && index < (int)tools_.size()) {
        if (active_tool_ >= 0 && active_tool_ < (int)tools_.size()) {
            tools_[active_tool_].tool->on_deactivate();
        }
        active_tool_ = index;
    }
}

Tool* Toolbox::get_active_tool() {
    if (active_tool_ >= 0 && active_tool_ < (int)tools_.size()) {
        return tools_[active_tool_].tool.get();
    }
    return nullptr;
}

void Toolbox::render_tool_buttons() {
    for (int i = 0; i < (int)tools_.size(); ++i) {
        bool is_active = (i == active_tool_);
        if (ImGui::Button(tools_[i].name.c_str())) {
            set_active_tool(i);
        }
        if (i < (int)tools_.size() - 1) {
            ImGui::SameLine();
        }
    }
}

void Toolbox::render_properties(Grid& grid, Polygon& polygon) {
    if (active_tool_ < 0 || active_tool_ >= (int)tools_.size()) return;

    Tool* tool = tools_[active_tool_].tool.get();
    if (!tool) return;

    // Instructions first, above the controls they describe. TextWrapped rather than Text: the
    // panel is a few hundred pixels wide and these are sentences, not labels.
    if (!tool->get_tooltip().empty()) {
        ImGui::TextWrapped("%s", tool->get_tooltip().c_str());
        ImGui::Separator();
    }

    PropertySet& props = tool->get_properties();

    for (auto& [id, prop] : props.getProperties()) {
        switch (prop.type) {
            case PropertyType::Float: {
                float val = std::stof(prop.value);
                if (ImGui::InputFloat(prop.name.c_str(), &val, 0.1f, 1.0f, "%.2f")) {
                    if (props.setProperty(id, std::to_string(val))) {
                        tool->on_property_changed(id, grid, polygon);
                    }
                }
                if (!prop.error_msg.empty()) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", prop.error_msg.c_str());
                }
                break;
            }

            case PropertyType::Int: {
                int val = std::stoi(prop.value);
                if (ImGui::InputInt(prop.name.c_str(), &val)) {
                    if (props.setProperty(id, std::to_string(val))) {
                        tool->on_property_changed(id, grid, polygon);
                    }
                }
                if (!prop.error_msg.empty()) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", prop.error_msg.c_str());
                }
                break;
            }

            case PropertyType::String: {
                // Label above the box, not as the widget label: ImGui draws widget labels to the
                // right of the control, and with a full-width box that puts the text off past the
                // panel edge. Matches the ReadOnly branch below.
                ImGui::Text("%s", prop.name.c_str());

                // Edited in place through a resize callback rather than copied into a fixed
                // buffer. The old char[1024] silently truncated any longer paste and then wrote
                // the truncated text straight back through parse_openscad_string, destroying the
                // polygon it was meant to load.
                Property* editable = props.getPropertyPtr(id);
                if (!editable) break;

                if (ImGui::InputTextMultiline(("##" + id).c_str(),
                        editable->value.data(), editable->value.capacity() + 1,
                        ImVec2(-1, 100),
                        ImGuiInputTextFlags_CallbackResize,
                        input_text_resize, &editable->value)) {
                    tool->on_property_changed(id, grid, polygon);
                }
                break;
            }

            case PropertyType::Bool: {
                bool val = (prop.value == "true");
                if (ImGui::Checkbox(prop.name.c_str(), &val)) {
                    if (props.setProperty(id, val ? "true" : "false")) {
                        tool->on_property_changed(id, grid, polygon);
                    }
                }
                break;
            }

            case PropertyType::ReadOnly: {
                ImGui::Text("%s", prop.name.c_str());
                // size() + 1: ImGui's buf_size counts the NUL terminator, so passing size()
                // clipped the last character -- and passed 0 for an empty value.
                ImGui::InputTextMultiline(("##" + id).c_str(),
                    const_cast<char*>(prop.value.c_str()), prop.value.size() + 1,
                    ImVec2(-1, 100), ImGuiInputTextFlags_ReadOnly);
                break;
            }
        }
    }

    // After the properties, so a button reads as acting on the box above it.
    for (const Action& action : tool->get_actions()) {
        if (ImGui::Button(action.label.c_str())) {
            tool->on_action(action.id, grid, polygon);
        }
    }
}
