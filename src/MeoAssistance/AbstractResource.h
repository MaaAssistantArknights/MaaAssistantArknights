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

#include <string>

namespace asst {
    class AbstractResource {
    public:
        AbstractResource() = default;
        AbstractResource(const AbstractResource& rhs) = delete;
        AbstractResource(AbstractResource&& rhs) noexcept = delete;

        virtual ~AbstractResource() = default;

        virtual bool load(const std::string& filename) = 0;
        virtual const std::string& get_last_error() const noexcept {
            return m_last_error;
        }

        AbstractResource& operator=(const AbstractResource& rhs) = delete;
        AbstractResource& operator=(AbstractResource&& rhs) noexcept = delete;

    protected:
        std::string m_last_error;
    };
}
