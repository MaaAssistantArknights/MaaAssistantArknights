@ stdcall AsstSetUserDir(ptr) WineShimAsstSetUserDir
@ stdcall AsstLoadResource(ptr) WineShimAsstLoadResource
@ stdcall AsstSetStaticOption(long ptr) WineShimAsstSetStaticOption

@ stdcall AsstCreate() WineShimAsstCreate
@ stdcall AsstCreateEx(ptr ptr) WineShimAsstCreateEx
@ stdcall AsstDestroy(ptr) WineShimAsstDestroy

@ stdcall AsstSetInstanceOption(ptr long ptr) WineShimAsstSetInstanceOption
@ stdcall AsstConnect(ptr ptr ptr ptr) WineShimAsstConnect

@ stdcall AsstAppendTask(ptr ptr ptr) WineShimAsstAppendTask
@ stdcall AsstSetTaskParams(ptr long ptr) WineShimAsstSetTaskParams

@ stdcall AsstStart(ptr) WineShimAsstStart
@ stdcall AsstStop(ptr) WineShimAsstStop
@ stdcall AsstRunning(ptr) WineShimAsstRunning
@ stdcall AsstConnected(ptr) WineShimAsstConnected
@ stdcall AsstBackToHome(ptr) WineShimAsstBackToHome

@ stdcall AsstAsyncConnect(ptr ptr ptr ptr long) WineShimAsstAsyncConnect
@ stdcall AsstSetConnectionExtras(ptr ptr) WineShimAsstSetConnectionExtras
@ stdcall AsstAsyncClick(ptr long long long) WineShimAsstAsyncClick
@ stdcall AsstAsyncScreencap(ptr long) WineShimAsstAsyncScreencap

@ stdcall AsstGetImage(ptr ptr long) WineShimAsstGetImage
@ stdcall AsstGetUUID(ptr ptr long) WineShimAsstGetUUID
@ stdcall AsstGetTasksList(ptr ptr long) WineShimAsstGetTasksList
@ stdcall AsstGetNullSize() WineShimAsstGetNullSize

@ stdcall AsstGetVersion() WineShimAsstGetVersion
@ stdcall AsstLog(ptr ptr) WineShimAsstLog

@ cdecl dl_has_maacore()
