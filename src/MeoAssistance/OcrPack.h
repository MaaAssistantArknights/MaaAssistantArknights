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

#include <functional>
#include <memory>

#include "AsstDef.h"

class OcrLiteCaller;
namespace cv {
    class Mat;
}

namespace asst {
    class OcrPack final : public AbstractResource {
    public:
        OcrPack();
        virtual ~OcrPack();

        virtual bool load(const std::string& dir) override;
        void set_param(int /*gpu_index*/, int thread_number);

        std::vector<TextRect> recognize(const cv::Mat& image, const TextRectProc& pred = nullptr);
        std::vector<TextRect> recognize(const cv::Mat& image, const Rect& roi, const TextRectProc& pred = nullptr);

    private:
        std::unique_ptr<OcrLiteCaller> m_ocr_ptr;
    };
}
