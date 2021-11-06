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

#include "AbstractResource.h"

namespace cv {
    class Mat;
}

namespace asst {
    class PenguinPack final : public AbstractResource {
    public:
        PenguinPack() = default;
        virtual ~PenguinPack() = default;

        virtual bool load(const std::string& dir) override;
        void set_language(const std::string& server);

        std::string recognize(const cv::Mat& image);

    private:
        bool load_json(const std::string& stage_path, const std::string& hash_path);
        bool load_templ(const std::string& item_id, const std::string& path);

        std::string m_language = "CN";
    };
}
