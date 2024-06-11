#include <dlfcn.h>
#define DL_FUNC_IMPL
#include "dl_maacore.h"

void dl_init_maacore()
{
    void* maacore = dlopen("libMaaCore.so", RTLD_NOW);

    if (!maacore) {
        return;
    }

    FILL_DL_FUNC(maacore, AsstSetUserDir)
    FILL_DL_FUNC(maacore, AsstLoadResource)
    FILL_DL_FUNC(maacore, AsstSetStaticOption)

    FILL_DL_FUNC(maacore, AsstCreate)
    FILL_DL_FUNC(maacore, AsstCreateEx)
    FILL_DL_FUNC(maacore, AsstDestroy)

    FILL_DL_FUNC(maacore, AsstSetInstanceOption)
    FILL_DL_FUNC(maacore, AsstConnect)

    FILL_DL_FUNC(maacore, AsstAppendTask)
    FILL_DL_FUNC(maacore, AsstSetTaskParams)

    FILL_DL_FUNC(maacore, AsstStart)
    FILL_DL_FUNC(maacore, AsstStop)
    FILL_DL_FUNC(maacore, AsstRunning)
    FILL_DL_FUNC(maacore, AsstConnected)
    FILL_DL_FUNC(maacore, AsstBackToHome)

    FILL_DL_FUNC(maacore, AsstAsyncConnect)
    FILL_DL_FUNC(maacore, AsstSetConnectionExtras)
    FILL_DL_FUNC(maacore, AsstAsyncClick)
    FILL_DL_FUNC(maacore, AsstAsyncScreencap)

    FILL_DL_FUNC(maacore, AsstGetImage)
    FILL_DL_FUNC(maacore, AsstGetUUID)
    FILL_DL_FUNC(maacore, AsstGetTasksList)
    FILL_DL_FUNC(maacore, AsstGetNullSize)

    FILL_DL_FUNC(maacore, AsstGetVersion)
    FILL_DL_FUNC(maacore, AsstLog)
}

#include <windef.h>

int __cdecl dl_has_maacore()
{
    return dl_AsstCreateEx != NULL;
}
