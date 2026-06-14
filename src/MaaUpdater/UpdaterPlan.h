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
// File I/O & string helpers
// ---------------------------------------------------------------------------

std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode);
bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason);
std::wstring Utf8ToWide(const std::string& s);

// ---------------------------------------------------------------------------
// Plan loading
// ---------------------------------------------------------------------------

bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson = nullptr);
