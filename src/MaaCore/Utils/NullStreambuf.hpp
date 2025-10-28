#pragma once

#include <iostream>

namespace asst::utils
{
class NullStreambuf : public std::streambuf
{
protected:
    int overflow(int c) override { return c; }

    std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
};
}

// namespace asst::utils
