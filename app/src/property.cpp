#include "property.h"
#include <stdexcept>
#include <sstream>

void PropertySet::addProperty(const std::string& id, const std::string& name,
                              PropertyType type, const std::string& default_value) {
    Property prop;
    prop.id = id;
    prop.name = name;
    prop.type = type;
    prop.value = default_value;
    properties[id] = prop;
}

std::string PropertySet::getProperty(const std::string& id) const {
    auto it = properties.find(id);
    if (it == properties.end()) {
        return "";
    }
    return it->second.value;
}

bool PropertySet::setProperty(const std::string& id, const std::string& value) {
    auto it = properties.find(id);
    if (it == properties.end()) {
        return false;
    }

    Property& prop = it->second;

    if (prop.type == PropertyType::ReadOnly) {
        prop.error_msg = "Property is read-only";
        return false;
    }

    if (prop.type == PropertyType::Float) {
        try {
            float fval = std::stof(value);
            if (fval < prop.min_val || fval > prop.max_val) {
                prop.error_msg = "Value out of range [" +
                                std::to_string(prop.min_val) + ", " +
                                std::to_string(prop.max_val) + "]";
                return false;
            }
            prop.error_msg.clear();
            prop.value = value;
            return true;
        } catch (...) {
            prop.error_msg = "Invalid float value";
            return false;
        }
    }

    if (prop.type == PropertyType::Int) {
        try {
            int ival = std::stoi(value);
            if (ival < prop.min_val || ival > prop.max_val) {
                prop.error_msg = "Value out of range";
                return false;
            }
            prop.error_msg.clear();
            prop.value = value;
            return true;
        } catch (...) {
            prop.error_msg = "Invalid integer value";
            return false;
        }
    }

    if (prop.type == PropertyType::Bool) {
        if (value == "true" || value == "false") {
            prop.error_msg.clear();
            prop.value = value;
            return true;
        }
        prop.error_msg = "Invalid boolean value";
        return false;
    }

    prop.error_msg.clear();
    prop.value = value;
    return true;
}

float PropertySet::getFloat(const std::string& id) const {
    auto it = properties.find(id);
    if (it == properties.end()) return 0.0f;
    try {
        return std::stof(it->second.value);
    } catch (...) {
        return 0.0f;
    }
}

int PropertySet::getInt(const std::string& id) const {
    auto it = properties.find(id);
    if (it == properties.end()) return 0;
    try {
        return std::stoi(it->second.value);
    } catch (...) {
        return 0;
    }
}

bool PropertySet::getBool(const std::string& id) const {
    auto it = properties.find(id);
    if (it == properties.end()) return false;
    return it->second.value == "true";
}

Property* PropertySet::getPropertyPtr(const std::string& id) {
    auto it = properties.find(id);
    if (it == properties.end()) return nullptr;
    return &it->second;
}
