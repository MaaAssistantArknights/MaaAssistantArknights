#include <cstring>
#include <iostream>
#include <chrono>

#include <Windows.h>

int main()
{
	HANDLE parent_read = nullptr;
	HANDLE parent_write = nullptr;
	HANDLE child_read = nullptr;
	HANDLE child_write = nullptr;

	// 安全属性描述符
	SECURITY_ATTRIBUTES sa_out_pipe = { 0 };
	sa_out_pipe.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa_out_pipe.lpSecurityDescriptor = NULL;
	sa_out_pipe.bInheritHandle = TRUE;

	constexpr int PipeBuffSize = 1048576;
	// 创建管道，父进程读-子进程写
	BOOL pipe_ret1 = ::CreatePipe(&parent_read, &child_write, &sa_out_pipe, PipeBuffSize);
	// 创建管道，父进程写-子进程读
	BOOL pipe_ret2 = ::CreatePipe(&child_read, &parent_write, &sa_out_pipe, PipeBuffSize);

	STARTUPINFOA startup_info = { 0 };			// 启动信息结构体
	startup_info.cb = sizeof(STARTUPINFO);
	startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_HIDE;
	// 重定向子进程的读写
	startup_info.hStdInput = child_read;
	startup_info.hStdOutput = child_write;
	startup_info.hStdError = child_write;

	PROCESS_INFORMATION process_info = { 0 };	// 进程信息结构体

	std::string adb = "adb -s 127.0.0.1:7555 shell echo hello";
	BOOL create_ret = ::CreateProcessA(NULL, const_cast<LPSTR>(adb.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_info);
	std::cout << "create: " << create_ret << std::endl;

	std::string pipe_str;
	const std::string end_flag = "root@x86_64:/ # ";
	DWORD process_ret = ::WaitForSingleObject(process_info.hProcess, 0);
	while (process_ret == WAIT_TIMEOUT) {
		DWORD read_num = 0;
		DWORD std_num = 0;
		DWORD write_num = 0;
		//std::string cmd = "screencap -p\r\n";
		//WriteFile(parent_write, cmd.c_str(), cmd.size(), &write_num, NULL);

		auto time = std::chrono::system_clock::now();
		while (process_ret == WAIT_TIMEOUT) {
			while (::PeekNamedPipe(parent_read, NULL, 0, NULL, &read_num, NULL) && read_num > 0) {
				//std::cout << read_num << std::endl;
				char* pipe_buffer = new char[read_num];

				BOOL read_ret = ::ReadFile(parent_read, pipe_buffer, read_num, &std_num, NULL);

				if (read_ret) {
					pipe_str += std::string(pipe_buffer, std_num);
				}

				delete[] pipe_buffer;

			}
			if (pipe_str.size() >= end_flag.size()
				&& pipe_str.compare(pipe_str.size() - end_flag.size(), end_flag.size(), end_flag) == 0) {
				break;

			}
			process_ret = ::WaitForSingleObject(process_info.hProcess, 0);

		}
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count() << "ms" << std::endl;

		std::cout << "size: " << pipe_str.size() << std::endl;
		if (!pipe_str.empty() && pipe_str.size() < 4096) {
			std::cout << pipe_str << std::endl;
		}
		pipe_str.clear();
	}
	std::cout << "process ret" << process_ret << std::endl;

	DWORD ret = -1;
	::GetExitCodeProcess(process_info.hProcess, &ret);

	::CloseHandle(process_info.hProcess);
	::CloseHandle(process_info.hThread);

	::CloseHandle(parent_read);
	::CloseHandle(parent_write);
	::CloseHandle(child_read);
	::CloseHandle(child_write);



	return 0;
}