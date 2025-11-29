#pragma once

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define NOMINMAX
#include <dbghelp.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "dbghelp.lib")
#undef NOMINMAX
#elif defined(__unix__) || defined(__APPLE__)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#endif

#include <format>
#include <iostream>
#include <string>

#include "Logger.hpp"

namespace asst::utils
{
class ExceptionStacktrace
{
public:
    static uintptr_t get_base_address()
    {
#ifdef _WIN32
        // 获取 MaaCore.dll 的基址（通过当前函数地址定位所属模块）
        HMODULE hModule = NULL;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&get_base_address),
            &hModule);
        return reinterpret_cast<uintptr_t>(hModule);
#elif defined(__APPLE__)
        // MacOS: 通过当前函数地址查找所属模块
        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(&get_base_address), &info)) {
            return reinterpret_cast<uintptr_t>(info.dli_fbase);
        }
        // 回退到主模块
        const struct mach_header* header = _dyld_get_image_header(0);
        return reinterpret_cast<uintptr_t>(header);
#else // Linux and other Unix-like systems
      // 通过当前函数地址查找所属模块基址
        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(&get_base_address), &info)) {
            return reinterpret_cast<uintptr_t>(info.dli_fbase);
        }
        // 回退：从/proc/self/maps读取
        std::ifstream maps("/proc/self/maps");
        std::string line;
        if (std::getline(maps, line)) {
            uintptr_t start_addr = 0;
            std::istringstream iss(line);
            std::string addr_range;
            iss >> addr_range;
            start_addr = std::stoull(addr_range.substr(0, addr_range.find('-')), nullptr, 16);
            return start_addr;
        }
        return 0;
#endif
    }
#ifdef _WIN32
    // Windows 异常堆栈跟踪实现
    static std::string capture_exception_stack_trace(PEXCEPTION_POINTERS pExceptionInfo)
    {
        std::string result;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        // 初始化符号处理器
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        if (!SymInitialize(process, NULL, TRUE)) {
            return "Failed to initialize symbol handler";
        }

        // 设置上下文
        CONTEXT* context = pExceptionInfo->ContextRecord;
        STACKFRAME64 frame = {};
        DWORD imageType;

#ifdef _M_IX86
        imageType = IMAGE_FILE_MACHINE_I386;
        frame.AddrPC.Offset = context->Eip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context->Ebp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context->Esp;
        frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
        imageType = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context->Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context->Rsp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context->Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
        imageType = IMAGE_FILE_MACHINE_IA64;
        frame.AddrPC.Offset = context->StIIP;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context->IntSp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrBStore.Offset = context->RsBSP;
        frame.AddrBStore.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context->IntSp;
        frame.AddrStack.Mode = AddrModeFlat;
#else
        return "Unsupported platform for stack walking";
#endif

        // 分配符号信息结构
        constexpr int MAX_NAME_LEN = 256;
        BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_NAME_LEN];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_NAME_LEN;

        // 行号信息
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        result += std::format("Exception Code: 0x{:08X}\n", pExceptionInfo->ExceptionRecord->ExceptionCode);
        result +=
            std::format("Exception Address: 0x{:016X}\n", (ULONG_PTR)pExceptionInfo->ExceptionRecord->ExceptionAddress);
        result += "Call Stack:\n";

        int frameNum = 0;
        while (StackWalk64(
            imageType,
            process,
            thread,
            &frame,
            context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL)) {
            if (frame.AddrPC.Offset == 0) {
                break;
            }

            result += std::format("  #{:2}: ", frameNum);

            // 获取符号信息
            DWORD64 displacement = 0;
            bool hasSymbol = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol);
            if (hasSymbol) {
                result += std::format("{}+0x{:X}", symbol->Name, displacement);
            }
            else {
                result += "<unknown>";
            }

            // 获取行号信息
            DWORD lineDisplacement = 0;
            bool hasLine = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line);
            if (hasLine) {
                result += std::format(" at {}:{}", line.FileName, line.LineNumber);
            }

            // 获取模块信息
            IMAGEHLP_MODULE64 moduleInfo = {};
            moduleInfo.SizeOfStruct = sizeof(moduleInfo);
            bool hasModule = SymGetModuleInfo64(process, frame.AddrPC.Offset, &moduleInfo);
            if (hasModule) {
                result += std::format(" [{}]", moduleInfo.ModuleName);
            }

            result += std::format(" (0x{:016X})\n", frame.AddrPC.Offset);

            frameNum++;
            if (frameNum > 64) { // 防止无限循环
                break;
            }
        }

        SymCleanup(process);
        return result;
    }

    // 在异常抛出点捕获堆栈跟踪
    static std::string capture_current_stack_trace()
    {
        HANDLE process = GetCurrentProcess();

        // 初始化符号处理器
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        if (!SymInitialize(process, NULL, TRUE)) {
            return "Failed to initialize symbol handler";
        }

        // 捕获当前堆栈
        void* stack[64];
        USHORT frames = CaptureStackBackTrace(0, 64, stack, NULL);

        // 分配符号信息结构
        constexpr int MAX_NAME_LEN = 256;
        BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_NAME_LEN];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_NAME_LEN;

        // 行号信息
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        std::vector<std::string> frame_strings;
        int index = 0;
        for (USHORT i = 0; i < frames && i < 64; i++) {
            std::string frame;
            DWORD64 address = (DWORD64)(stack[i]);

            // 获取符号信息
            DWORD64 displacement = 0;
            bool hasSymbol = SymFromAddr(process, address, &displacement, symbol);
            if (hasSymbol) {
                frame = std::format("{}+0x{:X}", symbol->Name, displacement);
            }
            else {
                frame = "<unknown>";
            }

            if (frame.starts_with("abort") || frame.starts_with("terminate")) {
                index = i;
            }

            // 获取行号信息
            DWORD lineDisplacement = 0;
            bool hasLine = SymGetLineFromAddr64(process, address, &lineDisplacement, &line);
            if (hasLine) {
                frame += std::format(" at {}:{}", line.FileName, line.LineNumber);
            }

            // 获取模块信息
            IMAGEHLP_MODULE64 moduleInfo = {};
            moduleInfo.SizeOfStruct = sizeof(moduleInfo);
            bool hasModule = SymGetModuleInfo64(process, address, &moduleInfo);
            if (hasModule) {
                frame += std::format(" [{}]", moduleInfo.ModuleName);
            }

            frame += std::format(" (0x{:016X})\n", address);
            frame_strings.emplace_back(std::move(frame));
        }

        SymCleanup(process);
        std::string result = "Current Stack Trace:\n";
        for (int i = index; i < frames; i++) {
            result += std::format("  #{:2}: {}", i - index, frame_strings[i]);
        }
        return result;
    }

#elif defined(__unix__) || defined(__APPLE__)
    // Unix/Linux/macOS 堆栈跟踪实现
    static std::string capture_current_stack_trace()
    {
        std::string result;
        result += "Current Stack Trace:\n";

        // 捕获堆栈帧
        constexpr int MAX_FRAMES = 64;
        void* buffer[MAX_FRAMES];
        int frames = backtrace(buffer, MAX_FRAMES);

        if (frames <= 0) {
            return "Failed to capture stack trace";
        }

        // 获取符号信息
        char** symbols = backtrace_symbols(buffer, frames);
        if (!symbols) {
            return "Failed to get symbol information";
        }

        // 解析并格式化每一帧
        for (int i = 0; i < frames && i < 64; i++) {
            result += std::format("  #{:2}: ", i);

            // 尝试解析符号名称
            std::string frame_info = demangle_symbol(symbols[i], buffer[i]);
            result += frame_info;
            result += "\n";
        }

        free(symbols);
        return result;
    }

private:
    // 辅助函数：解析和 demangle 符号名称
    static std::string demangle_symbol(const char* symbol_raw, void* address)
    {
        std::string result;

        // 尝试使用 dladdr 获取更详细的信息
        Dl_info info;
        if (dladdr(address, &info)) {
            // 获取模块名称
            if (info.dli_fname) {
                const char* fname = std::strrchr(info.dli_fname, '/');
                result += fname ? (fname + 1) : info.dli_fname;
                result += " ";
            }

            // 获取函数名称并 demangle
            if (info.dli_sname) {
                int status = 0;
                char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);

                if (status == 0 && demangled) {
                    result += demangled;
                    free(demangled);
                }
                else {
                    result += info.dli_sname;
                }

                // 计算偏移量
                if (info.dli_saddr) {
                    ptrdiff_t offset = static_cast<char*>(address) - static_cast<char*>(info.dli_saddr);
                    result += std::format("+0x{:X}", static_cast<size_t>(offset));
                }
            }
            else {
                result += "<unknown>";
            }

            // 添加地址
            result += std::format(" (0x{:016X})", reinterpret_cast<uintptr_t>(address));
        }
        else {
            // 如果 dladdr 失败，使用原始符号信息
            result = symbol_raw;
        }

        return result;
    }

#else
    // 不支持的平台
    static std::string capture_current_stack_trace() { return "Stack trace not supported on this platform"; }
#endif
};
} // namespace asst::utils
