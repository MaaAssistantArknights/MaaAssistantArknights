#pragma once
#include <string>
#include <string_view>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

namespace asst::utils
{
    inline std::string demangle(const char* name_from_typeid)
    {
#ifndef _MSC_VER
        int status = 0;
        std::size_t size = 0;
        char* p = abi::__cxa_demangle(name_from_typeid, NULL, &size, &status);
        if (!p) return name_from_typeid;
        std::string result(p);
        std::free(p);
        return result;
#else
        std::string_view temp(name_from_typeid);
        if (temp.substr(0, 6) == "class ") return std::string(temp.substr(6));
        if (temp.substr(0, 7) == "struct ") return std::string(temp.substr(7));
        if (temp.substr(0, 5) == "enum ") return std::string(temp.substr(5));
        return std::string(temp);
#endif
    }
}
