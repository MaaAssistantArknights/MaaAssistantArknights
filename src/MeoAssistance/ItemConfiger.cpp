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

#include "ItemConfiger.h"

#include <json.h>

bool asst::ItemConfiger::parse(const json::value& json) {
    for (const auto& [id, item_json] : json.as_object()) {
        std::string name = item_json.at("name_i18n").at("zh").as_string();
        m_item_name.emplace(id, std::move(name));
    }
    return true;
}