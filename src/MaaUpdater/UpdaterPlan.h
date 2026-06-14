#pragma once

#include <windows.h>

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Update plan structure
// ---------------------------------------------------------------------------

struct PendingUpdatePlan
{
    std::wstring packageType;
    std::vector<std::wstring> removeList;
    std::vector<std::wstring> moveList;
};

// ---------------------------------------------------------------------------
// JSON parsing helpers (internal, but exposed for test)
// ---------------------------------------------------------------------------

std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode);
bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason);
std::wstring Utf8ToWide(const std::string& s);
std::string ParseJsonString(const std::string& json, size_t& pos);
size_t SkipJsonWhitespace(const std::string& json, size_t pos);
size_t SkipJsonValue(const std::string& json, size_t pos);
size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key);
std::vector<std::wstring> ParseJsonStringArray(const std::string& json, const char* key);
std::wstring ParseJsonStringProperty(const std::string& json, const char* key);

// ---------------------------------------------------------------------------
// Plan loading & utilities
// ---------------------------------------------------------------------------

bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson = nullptr);
void PrintPlanEntries(const std::wstring& title, const std::vector<std::wstring>& entries);
int RunPlanParserTest(const std::wstring& initialPlanFile);
