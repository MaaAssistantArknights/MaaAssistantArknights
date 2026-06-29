#include "UpdaterPlan.h"
#include "UpdaterFile.h"

#include <meojson/json.hpp>

#include <cstring>

// ---------------------------------------------------------------------------
// File I/O helpers
// ---------------------------------------------------------------------------

std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode)
{
    return std::wstring(action) + L": " + path + L" (error=" + std::to_wstring(errorCode) + L")";
}

bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason)
{
    content.clear();
    failureReason.clear();

    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        failureReason = BuildFileIoFailureReason(L"Failed to open file", path, GetLastError());
        return false;
    }

    std::string buf;
    char chunk[4096];
    while (true) {
        DWORD chunkRead = 0;
        if (!ReadFile(hFile, chunk, static_cast<DWORD>(sizeof(chunk)), &chunkRead, nullptr)) {
            failureReason = BuildFileIoFailureReason(L"Failed to read file", path, GetLastError());
            CloseHandle(hFile);
            return false;
        }

        if (chunkRead == 0) {
            break;
        }

        buf.append(chunk, chunkRead);
    }

    CloseHandle(hFile);
    content.swap(buf);
    return true;
}

// ---------------------------------------------------------------------------
// String conversion
// ---------------------------------------------------------------------------

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 1) {
        return {};
    }
    std::wstring out(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

// ---------------------------------------------------------------------------
// Plan loading (using meojson)
// ---------------------------------------------------------------------------

namespace
{

std::vector<std::wstring> JsonArrayToWideStrings(const json::value& root, const char* key)
{
    std::vector<std::wstring> result;
    if (!root.contains(key)) {
        return result;
    }
    for (const auto& item : root.at(key).as_array()) {
        result.push_back(Utf8ToWide(item.as_string()));
    }
    return result;
}

} // anonymous namespace

bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson)
{
    outPlan = {};
    failureReason.clear();

    if (!PathExistsW(planFile)) {
        failureReason = L"Plan file not found: " + planFile;
        return false;
    }

    std::string planJson;
    if (!TryReadUtf8File(planFile, planJson, failureReason)) {
        return false;
    }

    if (rawJson != nullptr) {
        *rawJson = planJson;
    }

    if (planJson.empty()) {
        failureReason = L"Plan file is empty: " + planFile;
        return false;
    }

    auto parsed = json::parse(planJson);
    if (!parsed) {
        failureReason = L"Failed to parse plan JSON: " + planFile;
        return false;
    }

    const auto& root = *parsed;
    outPlan.packageType = Utf8ToWide(root.get("packageType", std::string {}));
    outPlan.removeList = JsonArrayToWideStrings(root, "removeList");
    outPlan.moveList = JsonArrayToWideStrings(root, "moveList");
    return true;
}

