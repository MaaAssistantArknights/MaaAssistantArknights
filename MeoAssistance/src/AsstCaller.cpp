#include "AsstCaller.h"
#include "Updater.h"

#include <string.h>

asst::Assistance* CreateAsst()
{
	return new asst::Assistance();
}

void DestoryAsst(asst::Assistance* p_asst)
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

	auto && ret = p_asst->catch_emulator();
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

bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char * buffer, int buffer_size)
{
	if (p_asst == NULL) {
		return false;
	}
	auto && ret = p_asst->get_param(type, param);
	if (!ret) {
		return false;
	}
	strcpy_s(buffer, buffer_size, ret.value().c_str());
	return true;
}

bool CheckVersionUpdate(char* tag_buffer, int tag_bufsize, char* html_url_buffer, int html_bufsize, char* body_buffer, int body_bufsize)
{
	bool ret = asst::Updater::get_instance().has_new_version();
	if (!ret) {
		return false;
	}
	const asst::VersionInfo & info = asst::Updater::get_instance().get_version_info();
	strcpy_s(tag_buffer, tag_bufsize, info.tag_name.c_str());
	strcpy_s(html_url_buffer, html_bufsize, info.html_url.c_str());
	strcpy_s(body_buffer, body_bufsize, info.body.c_str());
	return true;
}