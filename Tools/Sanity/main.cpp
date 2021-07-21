#include "Assistance.h"
#include "Updater.h"

int main(int argc, char** argv)
{
	using namespace asst;

	bool up = Updater::instance().has_new_version();
	if (up) {
		auto && info = Updater::instance().get_version_info();
		std::cout << "有新版本：" << info.tag_name << std::endl;
		std::cout << "地址：" << info.html_url << std::endl;
	}

	Assistance asst;

	auto ret = asst.setSimulator();
	if (!ret) {
		DebugTraceError("Can't Find Simulator or Permission denied.");
		getchar();
		return -1;
	}
	else {
		DebugTraceInfo("Find Simulator:", ret.value());
	}

	DebugTraceInfo("Start");
	asst.start("SanityBegin");

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}