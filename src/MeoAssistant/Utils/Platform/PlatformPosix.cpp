#if __has_include(<unistd.h>)
#include "PlatformPosix.h"
#include "Platform.h"

#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

static size_t get_page_size()
{
    return (size_t)sysconf(_SC_PAGESIZE);
}

const size_t asst::platform::page_size = get_page_size();

void* asst::platform::aligned_alloc(size_t len, size_t align)
{
    return ::aligned_alloc(len, align);
}

void asst::platform::aligned_free(void* ptr)
{
    ::free(ptr);
}

std::string asst::platform::call_command(const std::string& cmdline, bool* exit_flag)
{
    constexpr int PipeBuffSize = 4096;
    std::string pipe_str;
    auto pipe_buffer = std::make_unique<char[]>(PipeBuffSize);

    constexpr static int PIPE_READ = 0;
    constexpr static int PIPE_WRITE = 1;
    int pipe_in[2] = { 0 };
    int pipe_out[2] = { 0 };
    int pipe_in_ret = pipe(pipe_in);
    int pipe_out_ret = pipe(pipe_out);
    if (pipe_in_ret != 0 || pipe_out_ret != 0) {
        return {};
    }
    ::fcntl(pipe_out[PIPE_READ], F_SETFL, O_NONBLOCK);
    int exit_ret = 0;
    int child = ::fork();
    if (child == 0) {
        // child process
        ::dup2(pipe_in[PIPE_READ], STDIN_FILENO);
        ::dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
        ::dup2(pipe_out[PIPE_WRITE], STDERR_FILENO);

        // all these are for use by parent only
        ::close(pipe_in[PIPE_READ]);
        ::close(pipe_in[PIPE_WRITE]);
        ::close(pipe_out[PIPE_READ]);
        ::close(pipe_out[PIPE_WRITE]);

        exit_ret = execlp("sh", "sh", "-c", cmdline.c_str(), nullptr);
        ::exit(exit_ret);
    }
    else if (child > 0) {
        // parent process

        // close unused file descriptors, these are for child only
        ::close(pipe_in[PIPE_READ]);
        ::close(pipe_out[PIPE_WRITE]);

        do {
            ssize_t read_num = ::read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);

            while (read_num > 0) {
                pipe_str.append(pipe_buffer.get(), pipe_buffer.get() + read_num);
                read_num = ::read(pipe_out[PIPE_READ], pipe_buffer.get(), PipeBuffSize);
            };
        } while (::waitpid(child, &exit_ret, WNOHANG) == 0 && !(exit_flag && *exit_flag));

        ::close(pipe_in[PIPE_WRITE]);
        ::close(pipe_out[PIPE_READ]);
    }
    else {
        // failed to create child process
        ::close(pipe_in[PIPE_READ]);
        ::close(pipe_in[PIPE_WRITE]);
        ::close(pipe_out[PIPE_READ]);
        ::close(pipe_out[PIPE_WRITE]);
    }
    return pipe_str;
}

#endif
