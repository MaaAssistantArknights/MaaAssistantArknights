#include "UserConfiger.h"

#include <fstream>

#include <meojson/json.h>

constexpr static const char* EmulatorPathKey = "emulatorPath";

bool asst::UserConfiger::load(const std::string& filename)
{
    AbstractConfiger::load(filename);
    m_filename = filename;
    return true;
}

bool asst::UserConfiger::set_emulator_path(const std::string& name, const std::string& path)
{
    m_emulators_path.emplace(name, path);
    return save();
}

bool asst::UserConfiger::parse(const json::value& json)
{
    if (json.exist(EmulatorPathKey)) {
        for (const auto& [name, path] : json.at(EmulatorPathKey).as_object()) {
            m_emulators_path.emplace(name, path.as_string());
        }
    }
    m_raw_json = json;

    return true;
}

bool asst::UserConfiger::save()
{
    for (const auto& [name, path] : m_emulators_path) {
        m_raw_json[EmulatorPathKey][name] = path;
    }

    std::ofstream ofs(m_filename, std::ios::out);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << m_raw_json.format();
    ofs.close();

    return true;
}