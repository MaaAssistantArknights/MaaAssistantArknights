#pragma once
#ifdef _WIN32

#include "SafeWindows.h"

namespace asst::win32
{
    bool CreateOverlappablePipe(HANDLE* read, HANDLE* write, SECURITY_ATTRIBUTES* secattr_read,
                                SECURITY_ATTRIBUTES* secattr_write, DWORD bufsize, bool overlapped_read,
                                bool overlapped_write);
}

#endif
