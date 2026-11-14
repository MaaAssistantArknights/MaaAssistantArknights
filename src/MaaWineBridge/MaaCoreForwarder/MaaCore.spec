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
@ stdcall AsstAttachWindow(ptr ptr int64 int64 int64) WineShimAsstAttachWindow
@ stdcall AsstAsyncAttachWindow(ptr ptr int64 int64 int64 long) WineShimAsstAsyncAttachWindow

@ stdcall AsstAsyncConnect(ptr ptr ptr ptr long) WineShimAsstAsyncConnect
@ stdcall AsstSetConnectionExtras(ptr ptr) WineShimAsstSetConnectionExtras
@ stdcall AsstAsyncClick(ptr long long long) WineShimAsstAsyncClick
@ stdcall AsstAsyncScreencap(ptr long) WineShimAsstAsyncScreencap

@ stdcall -ret64 AsstGetImage(ptr ptr int64) WineShimAsstGetImage
@ stdcall -ret64 AsstGetImageBgr(ptr ptr int64) WineShimAsstGetImageBgr
@ stdcall -ret64 AsstGetUUID(ptr ptr int64) WineShimAsstGetUUID
@ stdcall -ret64 AsstGetTasksList(ptr ptr int64) WineShimAsstGetTasksList
@ stdcall -ret64 AsstGetNullSize() WineShimAsstGetNullSize

@ stdcall AsstGetVersion() WineShimAsstGetVersion
@ stdcall AsstLog(ptr ptr) WineShimAsstLog

@ cdecl dl_has_maacore()
