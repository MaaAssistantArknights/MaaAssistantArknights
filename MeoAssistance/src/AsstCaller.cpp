#include "AsstCaller.h"


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

	p_asst->setParam(type, param, value);
}
