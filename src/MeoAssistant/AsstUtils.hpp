#pragma once

#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <memory.h>
#endif

namespace asst::utils
{
    class MultiReplacer
    {
        class Node
        {
            std::map<char, Node*> _sn; //不知道为啥这个地方用 map 比用 unordered_map 快一些

        public:
            typedef decltype(_sn)::iterator iterator;
            typedef decltype(_sn)::const_iterator const_iterator;
            Node* fail;
            int replace_len = 0;
            std::string replace_key;

            template<typename... _Args>
            auto emplace(_Args&&... __args)
            {
                return _sn.emplace(std::forward<_Args>(__args)...);
            }

            template<typename... _Args>
            auto insert(_Args&&... __args)
            {
                return _sn.insert(std::forward<_Args>(__args)...);
            }

            Node* const at(const int& x) const
            {
                return _sn.at(x);
            }

            bool exists(const char& c) const
            {
                return _sn.count(c) != 0;
            }

            iterator begin() { return _sn.begin(); }
            iterator end() { return _sn.end(); }
            const_iterator begin() const { return _sn.cbegin(); }
            const_iterator end() const { return _sn.cend(); }
        };

        std::vector<Node*> _allocated;
        Node* _root = _allocated.emplace_back(new Node());
        std::unordered_map<std::string, std::string> _replace_map;
        bool _failgetted = false;

        void _getfail()
        {
            std::queue<Node*> q;
            _root->fail = _root;
            for (auto c : *_root) {
                c.second->fail = _root;
                q.push(c.second);
            }
            while (!q.empty()) {
                auto x = q.front(); q.pop();
                for (auto c : *x->fail) {
                    if (x->exists(c.first)) {
                        x->at(c.first)->fail = c.second;
                        q.push(x->at(c.first));
                    }
                    else {
                        x->insert(c);
                    }
                }
            }
        }

        // 根据建立的自动机，将 str 替换后的结果存入 res
        const void _replace(const std::string& str, std::string& res) const
        {
            Node* p = _root;
            for (char c : str) {
                if (p->exists(c)) {
                    p = p->at(c);
                }
                else if (_root->exists(c)) {
                    p = _root->at(c);
                }
                else {
                    p = _root;
                }
                if (p->replace_len) {
                    res.erase(res.length() + 1 - p->replace_len);
                    res.append(_replace_map.at(p->replace_key));
                    p = _root; // different from ac_automation (short first)
                }
                else {
                    res += c;
                }
            }
        }

        // 根据建立的自动机，将 str 替换后的结果存入 res
        template<typename map_t>
        const void _replace(const std::string& str, std::string& res, const map_t& replace_map) const
        {
            Node* p = _root;
            for (char c : str) {
                if (p->exists(c)) {
                    p = p->at(c);
                }
                else if (_root->exists(c)) {
                    p = _root->at(c);
                }
                else {
                    p = _root;
                }
                if (p->replace_len && replace_map.count(p->replace_key)) {
                    res.erase(res.length() + 1 - p->replace_len);
                    res.append(replace_map.at(p->replace_key));
                    p = _root; // different from ac_automation (short first)
                }
                else {
                    res += c;
                }
            }
        }

    public:
        MultiReplacer() {}
        ~MultiReplacer()
        {
            for (auto x : _allocated) {
                delete(x);
            }
        }

        // 所有 insert 操作必须在 replace 操作之前，且尽量不要插入子串
        auto& insert(const std::string& str, const std::string& rep)
        {
            assert(!_failgetted);
            Node* p = _root;
            for (const char& c : str) {
                if (!p->exists(c)) {
                    p->emplace(c, _allocated.emplace_back(new Node()));
                    // p->emplace(c, new Node());
                    // 内存不回收快好多欸，要不别回收了吧（
                }
                p = p->at(c);
            }
            // if(p->replace_len) {
            // 	// Warning: replace again.
            // }
            p->replace_len = str.length();
            p->replace_key = str;
            _replace_map[str] = rep;
            return *this;
        }

        // 在插入过 str 的情况下，更改其对应的替换字符串
        auto& change(const std::string& str, const std::string& rep)
        {
            assert(_replace_map.count(str) != 0);
            _replace_map[str] = rep;
            return *this;
        }

        // 根据建立的自动机，将 str 替换后的字符串输出
        std::string replace(const std::string& str)
        {
            if (!_failgetted) {
                _getfail();
                _failgetted = true;
            }
            std::string res = std::string();
            _replace(str, res);
            return res;
        }
        template<typename map_t>
        std::string replace(const std::string& str, const map_t& replace_map)
        {
            if (!_failgetted) {
                _getfail();
                _failgetted = true;
            }
            std::string res = std::string();
            _replace(str, res, replace_map);
            return res;
        }
    };
    MultiReplacer multireplacer;
    inline void string_replace_init()
    {
        static bool multireplacer_inited = false;
        if (multireplacer_inited) return;
        multireplacer_inited = true;
        // 想个办法把这里的硬编码改改
        multireplacer.insert("[Adb]", "[Adb]")
            .insert("[AdbSerial]", "[AdbSerial]")
            .insert("[x1]", "[x1]")
            .insert("[y1]", "[y1]")
            .insert("[x2]", "[x2]")
            .insert("[y2]", "[y2]")
            .insert("[x]", "[x]")
            .insert("[y]", "[y]")
            .insert(":", ":")
            .insert(" ", " ")
            .insert("[duration]", "[duration]")
            .insert("[DisplayId]", "[DisplayId]")
            .insert("[NcPort]", "[NcPort]")
            .insert("[NcAddress]", "[NcAddress]");
    }

    // 我不知道一个替换能不能更快，先暂时不改了
    inline std::string string_replace_all(const std::string& src, const std::string& old_value, const std::string& new_value)
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

    inline std::string string_replace_all_batch(const std::string& src, const std::unordered_map<std::string, std::string>& replace_pairs)
    {
        string_replace_init();
        return multireplacer.replace(src, replace_pairs);
    }

    inline std::vector<std::string> string_split(const std::string& str, const std::string& delimiter)
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

    inline std::string get_format_time()
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
        struct timeval tv {};
        gettimeofday(&tv, nullptr);
        time_t nowtime = tv.tv_sec;
        struct tm* tm_info = localtime(&nowtime);
        strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
        sprintf(buff, "%s.%03ld", buff, tv.tv_usec / 1000);
#endif  // END _WIN32
        return buff;
    }

    inline std::string ansi_to_utf8(const std::string& ansi_str)
    {
#ifdef _WIN32
        const char* src_str = ansi_str.c_str();
        int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, nullptr, 0);
        const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
        auto wstr = new wchar_t[wstr_length];
        memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
        MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);

        len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        const std::size_t str_length = static_cast<std::size_t>(len) + 1;
        auto str = new char[str_length];
        memset(str, 0, sizeof(str[0]) * str_length);
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, nullptr, nullptr);
        std::string strTemp = str;

        delete[] wstr;
        wstr = nullptr;
        delete[] str;
        str = nullptr;

        return strTemp;
#else   // Don't fucking use gbk in linux!
        return ansi_str;
#endif
    }

    inline std::string utf8_to_ansi(const std::string& utf8_str)
    {
#ifdef _WIN32
        const char* src_str = utf8_str.c_str();
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, nullptr, 0);
        const std::size_t wsz_ansi_length = static_cast<std::size_t>(len) + 1;
        auto wsz_ansi = new wchar_t[wsz_ansi_length];
        memset(wsz_ansi, 0, sizeof(wsz_ansi[0]) * wsz_ansi_length);
        MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wsz_ansi, len);

        len = WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, nullptr, 0, nullptr, nullptr);
        const std::size_t sz_ansi_length = static_cast<std::size_t>(len) + 1;
        auto sz_ansi = new char[sz_ansi_length];
        memset(sz_ansi, 0, sizeof(sz_ansi[0]) * sz_ansi_length);
        WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, sz_ansi, len, nullptr, nullptr);
        std::string strTemp(sz_ansi);

        delete[] wsz_ansi;
        wsz_ansi = nullptr;
        delete[] sz_ansi;
        sz_ansi = nullptr;

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

    inline std::string load_file_without_bom(const std::string& filename)
    {
        std::ifstream ifs(filename, std::ios::in);
        if (!ifs.is_open()) {
            return {};
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

    inline std::string callcmd(const std::string& cmdline)
    {
        constexpr int PipeBuffSize = 4096;
        std::string pipe_str;
        auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);

#ifdef _WIN32
        SECURITY_ATTRIBUTES pipe_sec_attr = { 0 };
        pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        pipe_sec_attr.lpSecurityDescriptor = nullptr;
        pipe_sec_attr.bInheritHandle = TRUE;
        HANDLE pipe_read = nullptr;
        HANDLE pipe_child_write = nullptr;
        CreatePipe(&pipe_read, &pipe_child_write, &pipe_sec_attr, PipeBuffSize);

        STARTUPINFOA si = { 0 };
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = pipe_child_write;
        si.hStdError = pipe_child_write;

        PROCESS_INFORMATION pi = { nullptr };

        BOOL p_ret = CreateProcessA(nullptr, const_cast<LPSTR>(cmdline.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (p_ret) {
            DWORD peek_num = 0;
            DWORD read_num = 0;
            do {
                while (::PeekNamedPipe(pipe_read, nullptr, 0, nullptr, &peek_num, nullptr) && peek_num > 0) {
                    if (::ReadFile(pipe_read, pipe_buffer.get(), peek_num, &read_num, nullptr)) {
                        pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                    }
                }
            } while (::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT);

            DWORD exit_ret = 255;
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

            exit_ret = execlp("sh", "sh", "-c", cmdline.c_str(), nullptr);
            exit(exit_ret);
        }
        else if (child > 0) {
            // parent process

            // close unused file descriptors, these are for child only
            close(pipe_in[PIPE_READ]);
            close(pipe_out[PIPE_WRITE]);

            do {
                ssize_t read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);

                while (read_num > 0) {
                    pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                    read_num = read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);
                };
            } while (::waitpid(child, &exit_ret, WNOHANG) == 0);

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

    // template<typename T,
    //	typename = typename std::enable_if<std::is_constructible<T, std::string>::value>::type>
    //	std::string VectorToString(const std::vector<T>& vector, bool to_gbk = false) {
    //	if (vector.empty()) {
    //		return std::string();
    //	}
    //	static const std::string inter = ",  ";
    //	std::string str;
    //	for (const T& ele : vector) {
    //		if (to_gbk) {
    //			str += utils::utf8_to_ansi(ele) + inter;
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

    template <typename T, T Val, typename... Unused>
    struct integral_constant_at_template_instantiation : std::integral_constant<T, Val> {};
}

//  delete instantiation of template with message, when static_assert(false, Message) does not work
#define ASST_STATIC_ASSERT_FALSE(Message, ...) \
static_assert(::asst::utils::integral_constant_at_template_instantiation<bool, false, __VA_ARGS__>::value, Message)
