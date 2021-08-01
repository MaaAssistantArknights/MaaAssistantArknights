#pragma once

#include "AsstPort.h"

#ifdef __cplusplus
extern "C" {
#endif
namespace asst {
	class Assistance;
}

MEOAPI_PORT asst::Assistance* AsstCreate();
MEOAPI_PORT void AsstDestory(asst::Assistance* p_asst);
MEOAPI_PORT bool AsstCatchEmulator(asst::Assistance* p_asst);
MEOAPI_PORT void AsstStart(asst::Assistance* p_asst, const char* task);
MEOAPI_PORT void AsstStop(asst::Assistance* p_asst);
MEOAPI_PORT bool AsstSetParam(asst::Assistance* p_asst, const char* type, const char* param, const char* value);
MEOAPI_PORT bool AsstGetParam(asst::Assistance* p_asst, const char* type, const char* param, char * buffer, int buffer_size);
MEOAPI_PORT bool AsstRunOpenRecruit(asst::Assistance* p_asst, const int required_level[], bool set_time, char * result_buffer, int bufsize, int& maybe_level);

MEOAPI_PORT bool CheckVersionUpdate(char* tag_buffer, int tag_bufsize, char* html_url_buffer, int html_bufsize, char* body_buffer, int body_bufsize);

#ifdef __cplusplus
}
#endif