#pragma once

#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <memory.h>
#endif

namespace asst
{
    namespace utils
    {
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

        static std::vector<std::string> string_split(const std::string& str, const std::string& delimiter)
        {
            std::string::size_type pos1 = 0;
            std::string::size_type pos2 = str.find(delimiter);
            std::vector<std::string> result;

            while (std::string::npos != pos2) {
                result.emplace_back(str.substr(pos1, pos2 - pos1));

                pos1 = pos2 + delimiter.size();
                pos2 = str.find(delimiter, pos1);
            }
            if (pos1 != str.length())
                result.emplace_back(str.substr(pos1));

            return result;
        }

        static std::string get_format_time()
        {
            char buff[128] = { 0 };
#ifdef _WIN32
            SYSTEMTIME curtime;
            GetLocalTime(&curtime);
#ifdef _MSC_VER
            sprintf_s(buff, sizeof(buff),
#else   // ! _MSC_VER
            sprintf(buff,
#endif  // END _MSC_VER
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                      curtime.wYear, curtime.wMonth, curtime.wDay,
                      curtime.wHour, curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);

#else   // ! _WIN32
            struct timeval tv = { 0 };
            gettimeofday(&tv, NULL);
            time_t nowtime = tv.tv_sec;
            struct tm* tm_info = localtime(&nowtime);
            strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
            sprintf(buff, "%s.%03ld", buff, tv.tv_usec / 1000);
#endif  // END _WIN32
            return buff;
        }

        static std::string gbk_2_utf8(const std::string& gbk_str)
        {
#ifdef _WIN32
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
            if (wstr)
                delete[] wstr;
            if (str)
                delete[] str;
            return strTemp;
#else   // Don't fucking use gbk in linux!
            return gbk_str;
#endif
        }

        static std::string utf8_to_gbk(const std::string& utf8_str)
        {
#ifdef _WIN32
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
            if (wszGBK)
                delete[] wszGBK;
            if (szGBK)
                delete[] szGBK;
            return strTemp;
#else   // Don't fucking use gbk in linux!
            return utf8_str;
#endif
        }

        template <typename RetTy, typename ArgType>
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

            if (str.size() >= 3 && static_cast<uchar>(str.at(0)) == Bom_0 && static_cast<uchar>(str.at(1)) == Bom_1 && static_cast<uchar>(str.at(2)) == Bom_2) {
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
                unsigned long long x = strtoull(hash1.substr(i, 16).c_str(), NULL, 16) ^ strtoull(hash2.substr(i, 16).c_str(), NULL, 16);
                while (x) {
                    dist++;
                    x = x & (x - 1);
                }
            }
            return dist;
        }

        static std::string callcmd(const std::string& cmdline)
        {
            constexpr int BuffSize = 4096;
            std::string pipe_str;
#ifdef _WIN32
            SECURITY_ATTRIBUTES pipe_sec_attr = { 0 };
            pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
            pipe_sec_attr.lpSecurityDescriptor = nullptr;
            pipe_sec_attr.bInheritHandle = TRUE;
            HANDLE pipe_read = nullptr;
            HANDLE pipe_child_write = nullptr;
            CreatePipe(&pipe_read, &pipe_child_write, &pipe_sec_attr, BuffSize);

            STARTUPINFOA si = { 0 };
            si.cb = sizeof(STARTUPINFO);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;
            si.hStdOutput = pipe_child_write;
            si.hStdError = pipe_child_write;

            PROCESS_INFORMATION pi = { 0 };

            BOOL p_ret = CreateProcessA(NULL, const_cast<LPSTR>(cmdline.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
            if (p_ret) {
                DWORD read_num = 0;
                DWORD std_num = 0;
                do {
                    while (::PeekNamedPipe(pipe_read, NULL, 0, NULL, &read_num, NULL) && read_num > 0) {
                        char* pipe_buffer = new char[read_num];
                        BOOL read_ret = ::ReadFile(pipe_read, pipe_buffer, read_num, &std_num, NULL);
                        if (read_ret) {
                            pipe_str.append(pipe_buffer, pipe_buffer + std_num);
                        }
                        if (pipe_buffer != nullptr) {
                            delete[] pipe_buffer;
                            pipe_buffer = nullptr;
                        }
                    }
                } while (::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT);

                DWORD exit_ret = -1;
                ::GetExitCodeProcess(pi.hProcess, &exit_ret);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            ::CloseHandle(pipe_read);
            ::CloseHandle(pipe_child_write);

#else
            constexpr static int PIPE_READ = 0;
            constexpr static int PIPE_WRITE = 1;
            int pipe_in[2] = { 0 };
            int pipe_out[2] = { 0 };
            int pipe_in_ret = pipe(pipe_in);
            int pipe_out_ret = pipe(pipe_out);
            fcntl(pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);
            int exit_ret = 0;
            int child = fork();
            if (child == 0) {
                // child process
                dup2(pipe_in[PIPE_READ], STDIN_FILENO);
                dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
                dup2(pipe_out[PIPE_WRITE], STDERR_FILENO);

                // all these are for use by parent only
                close(pipe_in[PIPE_READ]);
                close(pipe_in[PIPE_WRITE]);
                close(pipe_out[PIPE_READ]);
                close(pipe_out[PIPE_WRITE]);

                exit_ret = execlp("sh", "sh", "-c", cmdline.c_str(), NULL);
                exit(exit_ret);
            }
            else if (child > 0) {
                // parent process

                // close unused file descriptors, these are for child only
                close(pipe_in[PIPE_READ]);
                close(pipe_out[PIPE_WRITE]);

                std::unique_ptr<char> pipe_buffer(new char[BuffSize + 1]);
                do {
                    memset(pipe_buffer.get(), 0, BuffSize);
                    ssize_t read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), BuffSize);

                    while (read_num > 0) {
                        pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                        read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), BuffSize);
                    };
                } while (::waitpid(child, NULL, WNOHANG) == 0);

                close(pipe_in[PIPE_WRITE]);
                close(pipe_out[PIPE_READ]);
            }
            else {
                // failed to create child process
                close(pipe_in[PIPE_READ]);
                close(pipe_in[PIPE_WRITE]);
                close(pipe_out[PIPE_READ]);
                close(pipe_out[PIPE_WRITE]);
            }
#endif
            return pipe_str;
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
