#include <AsstCaller.h>

#include "dlshim.h"

DEFINE_DL_FUNC(AsstSetUserDir)
DEFINE_DL_FUNC(AsstLoadResource)
DEFINE_DL_FUNC(AsstSetStaticOption)

DEFINE_DL_FUNC(AsstCreate)
DEFINE_DL_FUNC(AsstCreateEx)
DEFINE_DL_FUNC(AsstDestroy)

DEFINE_DL_FUNC(AsstSetInstanceOption)
DEFINE_DL_FUNC(AsstConnect)

DEFINE_DL_FUNC(AsstAppendTask)
DEFINE_DL_FUNC(AsstSetTaskParams)

DEFINE_DL_FUNC(AsstStart)
DEFINE_DL_FUNC(AsstStop)
DEFINE_DL_FUNC(AsstRunning)
DEFINE_DL_FUNC(AsstConnected)
DEFINE_DL_FUNC(AsstBackToHome)

DEFINE_DL_FUNC(AsstAsyncConnect)
DEFINE_DL_FUNC(AsstSetConnectionExtras)
DEFINE_DL_FUNC(AsstAsyncClick)
DEFINE_DL_FUNC(AsstAsyncScreencap)

DEFINE_DL_FUNC(AsstGetImage)
DEFINE_DL_FUNC(AsstGetUUID)
DEFINE_DL_FUNC(AsstGetTasksList)
DEFINE_DL_FUNC(AsstGetNullSize)

DEFINE_DL_FUNC(AsstGetVersion)
DEFINE_DL_FUNC(AsstLog)

void dl_init_maacore();
