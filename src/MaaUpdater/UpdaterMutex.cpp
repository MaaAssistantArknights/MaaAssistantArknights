#include "UpdaterMutex.h"
#include "UpdaterLog.h"

HANDLE AcquireUpdateMutex(const std::wstring& mutexName)
{
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());
    if (hMutex == nullptr) {
        WriteLog((L"CreateMutexW failed, error=" + std::to_wstring(GetLastError())));
        return nullptr;
    }

    DWORD waitResult = WaitForSingleObject(hMutex, UPDATE_MUTEX_TIMEOUT_MS);

    if (waitResult == WAIT_OBJECT_0) {
        WriteLog((L"Mutex acquired: " + mutexName));
        return hMutex;
    }

    if (waitResult == WAIT_ABANDONED) {
        // Previous MAA instance terminated abnormally; we now own the mutex.
        WriteLog((L"Mutex acquired after WAIT_ABANDONED (previous instance crashed): " + mutexName));
        return hMutex;
    }

    if (waitResult == WAIT_TIMEOUT) {
        WriteLog((L"Mutex acquisition timed out after " + std::to_wstring(UPDATE_MUTEX_TIMEOUT_MS) + L"ms: " + mutexName));
    } else {
        WriteLog((L"WaitForSingleObject failed, error=" + std::to_wstring(GetLastError())));
    }

    CloseHandle(hMutex);
    return nullptr;
}

void ReleaseUpdateMutex(HANDLE hMutex)
{
    if (hMutex == nullptr || hMutex == INVALID_HANDLE_VALUE) {
        return;
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}
