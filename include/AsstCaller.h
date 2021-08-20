#pragma once

#include "AsstPort.h"

#ifdef __cplusplus
extern "C" {
#endif
	namespace asst {
		class Assistance;
	}
	typedef void (MEO_CALL* AsstCallback)(int msg, const char* detail_json, void* custom_arg);

	MEOAPI_PORT asst::Assistance* AsstCreate();
	MEOAPI_PORT asst::Assistance* AsstCreateEx(AsstCallback callback, void* custom_arg);
	MEOAPI_PORT void AsstDestory(asst::Assistance* p_asst);
	MEOAPI_PORT bool AsstCatchEmulator(asst::Assistance* p_asst);
	MEOAPI_PORT bool AsstStart(asst::Assistance* p_asst, const char* task);
	MEOAPI_PORT void AsstStop(asst::Assistance* p_asst);
	MEOAPI_PORT bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);
	MEOAPI_PORT bool AsstRunOpenRecruit(asst::Assistance* p_asst, const int required_level[], bool set_time);
	MEOAPI_PORT bool AsstStartInfrast(asst::Assistance* p_asst);
	MEOAPI_PORT bool AsstTestOcr(asst::Assistance* p_asst, const char** text_array, int array_size, bool need_click);

	MEOAPI_PORT bool CheckVersionUpdate(char* tag_buffer, int tag_bufsize, char* html_url_buffer, int html_bufsize, char* body_buffer, int body_bufsize);

	MEOAPI_PORT bool AsstTestSwipe(asst::Assistance* p_asst, int x1, int y1, int x2, int y2);
#ifdef __cplusplus
}
#endif