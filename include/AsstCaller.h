#pragma once

#include "AsstPort.h"

#ifdef __cplusplus
extern "C" {
#endif
	namespace asst {
		class Assistance;
	}
	typedef void (MEO_CALL* AsstCallback)(int msg, const char* detail_json, void* custom_arg);

	MEOAPI_PORT asst::Assistance* MEO_CALL AsstCreate();
	MEOAPI_PORT asst::Assistance* MEO_CALL AsstCreateEx(AsstCallback callback, void* custom_arg);
	void MEOAPI AsstDestory(asst::Assistance* p_asst);

	bool MEOAPI AsstCatchDefault(asst::Assistance* p_asst);
	bool MEOAPI AsstCatchEmulator(asst::Assistance* p_asst);
	bool MEOAPI AsstCatchUSB(asst::Assistance* p_asst);
	bool MEOAPI AsstCatchRemote(asst::Assistance* p_asst, const char* address);

	bool MEOAPI AsstStartSanity(asst::Assistance* p_asst);
	bool MEOAPI AsstStartVisit(asst::Assistance* p_asst);
	bool MEOAPI AsstStartProcessTask(asst::Assistance* p_asst, const char* task);
	bool MEOAPI AsstStartOpenRecruit(asst::Assistance* p_asst, const int required_level[], int required_len, bool set_time);
	bool MEOAPI AsstStartIndertifyOpers(asst::Assistance* p_asst);
	bool MEOAPI AsstStartInfrast(asst::Assistance* p_asst);
	bool MEOAPI AsstStartDebugTask(asst::Assistance* p_asst);

	void MEOAPI AsstStop(asst::Assistance* p_asst);
	bool MEOAPI AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);

	bool MEOAPI CheckVersionUpdate(char* tag_buffer, int tag_bufsize, char* html_url_buffer, int html_bufsize, char* body_buffer, int body_bufsize);
#ifdef __cplusplus
}
#endif