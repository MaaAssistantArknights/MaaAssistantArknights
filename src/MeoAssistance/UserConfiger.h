#pragma once

#include "AbstractConfiger.h"

#include <json_value.h>

namespace asst {
    class UserConfiger :public AbstractConfiger
    {
    public:
        virtual ~UserConfiger() = default;

        static UserConfiger& get_instance() {
            static UserConfiger unique_instance;
            return unique_instance;
        }

        virtual bool load(const std::string& filename) override;

        bool set_emulator_path(const std::string& name, const std::string& path);

    protected:
        virtual bool parse(const json::value& json) override;
        bool save();

        json::value m_raw_json;
        std::string m_filename;
    };

}