#include "AsstCaller.h"
#include "Updater.h"
#include "AsstAux.h"
#include "Assistance.h"

#include <string.h>

asst::Assistance* AsstCreate()
{
	return new asst::Assistance();
}

void AsstDestory(asst::Assistance* p_asst)
{
	if (p_asst == NULL) {
		return;
	}

	delete p_asst;
	p_asst = NULL;
}

bool AsstCatchEmulator(asst::Assistance* p_asst)
{
	if (p_asst == NULL) {
		return false;
	}

	auto&& ret = p_asst->catch_emulator();
	if (ret) {
		return true;
	}
	else {
		return false;
	}
}

void AsstStart(asst::Assistance* p_asst, const char* task)
{
	if (p_asst == NULL) {
	}

	p_asst->start(task);
}

void AsstStop(asst::Assistance* p_asst)
{
	if (p_asst == NULL) {
		return;
	}

	p_asst->stop();
}

bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value)
{
	if (p_asst == NULL) {
		return false;
	}

	return p_asst->set_param(type, param, value);
}

bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char* buffer, int buffer_size)
{
	if (p_asst == NULL) {
		return false;
	}
	auto&& ret = p_asst->get_param(type, param);
	if (!ret) {
		return false;
	}
	strcpy_s(buffer, buffer_size, ret.value().c_str());
	return true;
}

bool AsstRunOpenRecruit(asst::Assistance* p_asst, const int required_level[], bool set_time, char* result_buffer, int bufsize, int& maybe_level)
{
	if (p_asst == NULL) {
		return false;
	}
	int len = sizeof required_level / sizeof(int);
	std::vector<int> level_vector;
	level_vector.assign(required_level, required_level + len);
	auto&& ret = p_asst->open_recruit(level_vector, set_time);
	if (ret) {
		// <std::vector<std::pair<std::vector<std::string>, OperCombs>>>
		std::string result_str;
		maybe_level = 0;
		for (auto&& [tags, oper_combs] : ret.value())
		{
			result_str += std::to_string(oper_combs.min_level) + "ÐÇTags:  " 
				+ asst::VectorToString(tags, true) + "\n\t";
			for (auto&& oper : oper_combs.opers)
			{
				result_str += std::to_string(oper.level) + " - " + asst::Utf8ToGbk(oper.name) + "    ";
			}
			result_str += "\n\n";
			if (maybe_level == 0) {
				maybe_level = oper_combs.min_level;
			}
		}
		strcpy_s(result_buffer, bufsize, result_str.c_str());
		return true;
	}
	else {
		return false;
	}
}


bool CheckVersionUpdate(char* tag_buffer, int tag_bufsize, char* html_url_buffer, int html_bufsize, char* body_buffer, int body_bufsize)
{
	bool ret = asst::Updater::get_instance().has_new_version();
	if (!ret) {
		return false;
	}
	const asst::VersionInfo& info = asst::Updater::get_instance().get_version_info();
	strcpy_s(tag_buffer, tag_bufsize, info.tag_name.c_str());
	strcpy_s(html_url_buffer, html_bufsize, info.html_url.c_str());
	strcpy_s(body_buffer, body_bufsize, info.body.c_str());
	return true;
}