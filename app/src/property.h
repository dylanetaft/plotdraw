#pragma once

#include <string>
#include <map>
#include <cfloat>

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
public:
    void addProperty(const std::string& id, const std::string& name,
                     PropertyType type, const std::string& default_value = "");

    std::string getProperty(const std::string& id) const;
    bool setProperty(const std::string& id, const std::string& value);

    float getFloat(const std::string& id) const;
    int getInt(const std::string& id) const;
    bool getBool(const std::string& id) const;

    Property* getPropertyPtr(const std::string& id);
    const std::map<std::string, Property>& getProperties() const { return properties; }

private:
    std::map<std::string, Property> properties;
};
