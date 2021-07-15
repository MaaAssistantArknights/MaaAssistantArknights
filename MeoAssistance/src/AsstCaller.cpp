#include "AsstCaller.h"


asst::Assistance* CreateAsst()
{
	return new asst::Assistance();
}

void DestoryAsst(asst::Assistance* p_asst)
{
	if (p_asst != NULL) {
		delete p_asst;
		p_asst = NULL;
	}
}

bool AsstFindSimulator(asst::Assistance* p_asst)
{
	if (p_asst != NULL) {
		auto ret = p_asst->setSimulator();
		if (ret) {
			return true;
		}
		else {
			return false;
		}
	}
}

void AsstStart(asst::Assistance* p_asst, const char* task)
{
	if (p_asst != NULL) {
		p_asst->start(task);
	}
}

void AsstStop(asst::Assistance* p_asst)
{
	if (p_asst != NULL) {
		p_asst->stop();
	}
}