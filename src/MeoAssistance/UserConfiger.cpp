/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "UserConfiger.h"

#include <fstream>

#include <json.h>

constexpr static const char* EmulatorPathKey = "emulatorPath";

bool asst::UserConfiger::load(const std::string& filename) {
    AbstractConfiger::load(filename);
    m_filename = filename;
    return true;
}

bool asst::UserConfiger::set_emulator_path(const std::string& name, const std::string& path) {
    m_emulators_path.emplace(name, path);
    return save();
}

bool asst::UserConfiger::parse(const json::value& json) {
    if (json.exist(EmulatorPathKey)) {
        for (const auto& [name, path] : json.at(EmulatorPathKey).as_object()) {
            m_emulators_path.emplace(name, path.as_string());
        }
    }
    m_raw_json = json;

    return true;
}

bool asst::UserConfiger::save() {
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