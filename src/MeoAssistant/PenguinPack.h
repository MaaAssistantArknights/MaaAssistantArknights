#pragma once

#include "AbstractResource.h"

namespace cv
{
    class Mat;
}

namespace asst
{
    class PenguinPack final : public AbstractResource
    {
    public:
        PenguinPack() = default;
        virtual ~PenguinPack() = default;

        virtual bool load(const std::string& dir) override;
        void set_language(const std::string& server);

        std::string recognize(const cv::Mat image);

    private:
        bool load_json(const std::string& stage_path, const std::string& hash_path);
        bool load_templ(const std::string& item_id, const std::string& path);

        std::string m_language = "CN";
    };
}
