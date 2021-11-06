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

#pragma once

#include "AbstractConfiger.h"

#include <json_value.h>
#include <unordered_map>

namespace asst {
    class UserConfiger : public AbstractConfiger {
    public:
        virtual ~UserConfiger() = default;
        virtual bool load(const std::string& filename) override;

        bool set_emulator_path(const std::string& name, const std::string& path);
        const std::string& get_emulator_path(const std::string& name) const noexcept {
            if (auto iter = m_emulators_path.find(name);
                iter != m_emulators_path.cend()) {
                return iter->second;
            }
            else {
                static const std::string empty;
                return empty;
            }
        }
        const std::unordered_map<std::string, std::string>& get_emulator_path() const noexcept {
            return m_emulators_path;
        }

    protected:
        virtual bool parse(const json::value& json) override;
        bool save();

        json::value m_raw_json;
        std::string m_filename;
        std::unordered_map<std::string, std::string> m_emulators_path;
    };
}
