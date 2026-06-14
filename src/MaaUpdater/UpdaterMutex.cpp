#include "UpdaterMutex.h"
#include "UpdaterLog.h"

HANDLE AcquireUpdateMutex(const std::wstring& mutexName)
{
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());
    if (hMutex == nullptr) {
        WriteLogF(L"CreateMutexW failed, error=%lu", GetLastError());
        return nullptr;
    }

    DWORD waitResult = WaitForSingleObject(hMutex, UPDATE_MUTEX_TIMEOUT_MS);

    if (waitResult == WAIT_OBJECT_0) {
        WriteLogF(L"Mutex acquired: %s", mutexName);
        return hMutex;
    }

    if (waitResult == WAIT_ABANDONED) {
        // Previous MAA instance terminated abnormally; we now own the mutex.
        WriteLogF(L"Mutex acquired after WAIT_ABANDONED (previous instance crashed): %s", mutexName);
        return hMutex;
    }

    if (waitResult == WAIT_TIMEOUT) {
        WriteLogF(L"Mutex acquisition timed out after %lums: %s", UPDATE_MUTEX_TIMEOUT_MS, mutexName);
    } else {
        WriteLogF(L"WaitForSingleObject failed, error=%lu", GetLastError());
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
