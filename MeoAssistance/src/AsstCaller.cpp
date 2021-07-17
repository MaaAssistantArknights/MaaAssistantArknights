#include "AsstCaller.h"

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

bool AsstCatchSimulator(asst::Assistance* p_asst)
{
	if (p_asst == NULL) {
		return false;
	}

	auto ret = p_asst->setSimulator();
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

	return p_asst->setParam(type, param, value);
}

bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char * buffer, int buffer_size)
{
	if (p_asst == NULL) {
		return false;
	}
	auto ret = p_asst->getParam(type, param);
	if (!ret) {
		return false;
	}
	strcpy_s(buffer, buffer_size, ret.value().c_str());
	return true;
}