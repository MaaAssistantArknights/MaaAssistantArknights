#pragma once

#include <windows.h>

#include <string>

// ---------------------------------------------------------------------------
// Update mutex constants
// ---------------------------------------------------------------------------

constexpr DWORD UPDATE_MUTEX_TIMEOUT_MS = 3000;

// ---------------------------------------------------------------------------
// Update mutex helpers
// ---------------------------------------------------------------------------

// Acquires a named system mutex to prevent new MAA instances from starting
// while the update is in progress. Returns nullptr if the mutex cannot be
// acquired (e.g. another MAA instance holds the lock).
HANDLE AcquireUpdateMutex(const std::wstring& mutexName);
void ReleaseUpdateMutex(HANDLE hMutex);
