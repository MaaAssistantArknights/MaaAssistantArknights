#pragma once

#include <string>

namespace json
{
    class value;
}

namespace asst
{
    class PenguinUploader
    {
    public:
        static bool upload(const std::string& rec_res);

    private:
        static std::string cvt_json(const std::string& rec_res);
        static bool request_penguin(const std::string& body);
    };
}
