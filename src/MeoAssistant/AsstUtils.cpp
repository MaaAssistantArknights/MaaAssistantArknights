#include "AsstUtils.hpp"

#ifdef _WIN32

#include <mbctype.h>

std::string asst::utils::path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize;
    auto osstr = path.native();
    auto err = wcstombs_s(&mbsize, nullptr, 0, osstr.c_str(), osstr.size());
    if (err == 0) {
        std::string result(mbsize, 0);
        err = wcstombs_s(&mbsize, &result[0], mbsize, osstr.c_str(), osstr.size());
        if (err != 0) return {};
        return result.substr(0, mbsize - 1);
    }
    else {
        // cannot convert (CRT is not using UTF-8), fallback to short path name in ACP
        wchar_t short_path[MAX_PATH];
        auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
        if (shortlen == 0) return {};
        BOOL failed = FALSE;
        auto ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
        if (failed) return {};
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, &result[0], ansilen, nullptr, nullptr);
        return result;
    }
}

std::string asst::utils::path_to_ansi_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    BOOL failed = FALSE;
    auto osstr = path.native();
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), nullptr, 0, nullptr, &failed);
    if (!failed) {
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), &result[0], ansilen, nullptr, &failed);
        return result;
    }
    else {
        // contains character that cannot be converted, fallback to short path name in ACP
        wchar_t short_path[MAX_PATH];
        auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
        if (shortlen == 0) return {};
        ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
        if (failed) return {};
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, &result[0], ansilen, nullptr, nullptr);
        return result;
    }
}

asst::utils::os_string asst::utils::to_osstring(const std::string& utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), nullptr, 0);
    asst::utils::os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), &result[0], len);
    return result;
}

std::string asst::utils::from_osstring(const asst::utils::os_string& os_str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, os_str.c_str(), (int)os_str.size(), &result[0], len, nullptr, nullptr);
    return result;
}



#endif
