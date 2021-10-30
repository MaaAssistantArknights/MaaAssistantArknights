#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>

namespace asst {
    namespace utils {
        static std::string get_cur_dir()
        {
            static std::string cur_dir;
            if (cur_dir.empty()) {
                char exepath_buff[_MAX_PATH] = { 0 };
                ::GetModuleFileNameA(NULL, exepath_buff, _MAX_PATH);
                std::string exepath(exepath_buff);
                cur_dir = exepath.substr(0, exepath.find_last_of('\\') + 1);
            }
            return cur_dir;
        }

        static std::string get_resource_dir()
        {
            static std::string res_dir = get_cur_dir() + "resource\\";
            return res_dir;
        }

        static std::string string_replace_all(const std::string& src, const std::string& old_value, const std::string& new_value)
        {
            std::string str = src;
            for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
                if ((pos = str.find(old_value, pos)) != std::string::npos)
                    str.replace(pos, old_value.length(), new_value);
                else
                    break;
            }
            return str;
        }

        static std::string get_format_time()
        {
            SYSTEMTIME curtime;
            GetLocalTime(&curtime);
            char buff[64] = { 0 };
            sprintf_s(buff, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                curtime.wYear, curtime.wMonth, curtime.wDay,
                curtime.wHour, curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);
            return buff;
        }

        static std::string gbk_2_utf8(const std::string& gbk_str)
        {
            const char* src_str = gbk_str.c_str();
            int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
            wchar_t* wstr = new wchar_t[len + 1];
            memset(wstr, 0, len + 1);
            MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
            len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
            char* str = new char[len + 1];
            memset(str, 0, len + 1);
            WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
            std::string strTemp = str;
            if (wstr) delete[] wstr;
            if (str) delete[] str;
            return strTemp;
        }

        static std::string utf8_to_gbk(const std::string& utf8_str)
        {
            const char* src_str = utf8_str.c_str();
            int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
            wchar_t* wszGBK = new wchar_t[len + 1];
            memset(wszGBK, 0, len * 2 + 2);
            MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
            len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
            char* szGBK = new char[len + 1];
            memset(szGBK, 0, len + 1);
            WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
            std::string strTemp(szGBK);
            if (wszGBK) delete[] wszGBK;
            if (szGBK) delete[] szGBK;
            return strTemp;
        }

        template<typename RetTy, typename ArgType>
        constexpr inline RetTy make_rect(const ArgType& rect)
        {
            return RetTy{ rect.x, rect.y, rect.width, rect.height };
        }

        static std::string load_file_without_bom(const std::string& filename)
        {
            std::ifstream ifs(filename, std::ios::in);
            if (!ifs.is_open()) {
                return std::string();
            }
            std::stringstream iss;
            iss << ifs.rdbuf();
            ifs.close();
            std::string str = iss.str();

            using uchar = unsigned char;
            constexpr static uchar Bom_0 = 0xEF;
            constexpr static uchar Bom_1 = 0xBB;
            constexpr static uchar Bom_2 = 0xBF;

            if (str.size() >= 3
                && static_cast<uchar>(str.at(0)) == Bom_0
                && static_cast<uchar>(str.at(1)) == Bom_1
                && static_cast<uchar>(str.at(2)) == Bom_2) {
                str.assign(str.begin() + 3, str.end());
                return str;
            }
            return str;
        }

        static int hamming(std::string hash1, std::string hash2)
        {
            constexpr static int HammingFlags = 64;

            hash1.insert(hash1.begin(), HammingFlags - hash1.size(), '0');
            hash2.insert(hash2.begin(), HammingFlags - hash2.size(), '0');
            int dist = 0;
            for (int i = 0; i < HammingFlags; i = i + 16) {
                unsigned long long x
                    = strtoull(hash1.substr(i, 16).c_str(), NULL, 16)
                    ^ strtoull(hash2.substr(i, 16).c_str(), NULL, 16);
                while (x) {
                    dist++;
                    x = x & (x - 1);
                }
            }
            return dist;
        }

        //template<typename T,
        //	typename = typename std::enable_if<std::is_constructible<T, std::string>::value>::type>
        //	std::string VectorToString(const std::vector<T>& vector, bool to_gbk = false) {
        //	if (vector.empty()) {
        //		return std::string();
        //	}
        //	static const std::string inter = ",  ";
        //	std::string str;
        //	for (const T& ele : vector) {
        //		if (to_gbk) {
        //			str += utils::utf8_to_gbk(ele) + inter;
        //		}
        //		else {
        //			str += ele + inter;
        //		}
        //	}

        //	if (!str.empty()) {
        //		str.erase(str.size() - inter.size(), inter.size());
        //	}
        //	return str;
        //}

        //template<typename T,
        //	typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        //	std::string VectorToString(const std::vector<T>& vector) {
        //	if (vector.empty()) {
        //		return std::string();
        //	}
        //	static const std::string inter = ",  ";
        //	std::string str;
        //	for (const T& ele : vector) {
        //		str += std::to_string(ele) + inter;
        //	}

        //	if (!str.empty()) {
        //		str.erase(str.size() - inter.size(), inter.size());
        //	}
        //	return str;
        //}
    }
}