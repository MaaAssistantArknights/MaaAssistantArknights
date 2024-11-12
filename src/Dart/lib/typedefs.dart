import "dart:ffi";

import "package:ffi/ffi.dart";

typedef Assistant = Pointer<Void>;
typedef AsstCallbackFunc = void Function(int, Pointer<Utf8>, Pointer<Void>);
typedef AsstCallbackNative = NativeFunction<Void Function(Int32, Pointer<Utf8>, Pointer<Void>)>;
// AsstCreateEx
typedef AsstCreateDartFunc = Assistant Function(Pointer<Int64>);
typedef AsstCreateDartNative
    = NativeFunction<Assistant Function(Pointer<Int64>)>;
typedef AsstCreateExFunc = Assistant Function(Pointer<AsstCallbackNative>, Pointer<Void>);
typedef AsstCreateExNative = NativeFunction<AsstCreateExFunc>;

// AsstCreate
typedef AsstCreateFunc = Assistant Function();
typedef AsstCreateNative = NativeFunction<AsstCreateFunc>;

// AsstDestroy
typedef AsstDestroyFunc = void Function(Assistant);
typedef AsstDestroyNative = NativeFunction<Void Function(Assistant)>;

// AsstConnect
typedef AsstConnectFunc = bool Function(
    Assistant, StringPtr, StringPtr, StringPtr);
typedef AsstConnectNative
    = NativeFunction<Bool Function(Assistant, StringPtr, StringPtr, StringPtr)>;

// AsstAppendTask
typedef AsstAppendTaskFunc = TaskId Function(Assistant, StringPtr, StringPtr);
typedef AsstAppendTaskNative
    = NativeFunction<TaskIdNative Function(Assistant, StringPtr, StringPtr)>;

// AsstSetTaskParams
typedef AsstSetTaskParamsFunc = bool Function(Assistant, TaskId, StringPtr);
typedef AsstSetTaskParamsNative
    = NativeFunction<Bool Function(Assistant, TaskIdNative, StringPtr)>;

// AsstStart
typedef AsstStartFunc = bool Function(Assistant);
typedef AsstStartNative = NativeFunction<Bool Function(Assistant)>;

// AsstStop
typedef AsstStopFunc = bool Function(Assistant);
typedef AsstStopNative = NativeFunction<Bool Function(Assistant)>;

// AsstCtrlerClick
typedef AsstCtrlerClickFunc = bool Function(Assistant, int, int, bool);
typedef AsstCtrlerClickNative
    = NativeFunction<Bool Function(Assistant, Int32, Int32, Bool)>;

// AsstGetVersion
typedef AsstGetVersionFunc = StringPtr Function();
typedef AsstGetVersionNative = NativeFunction<AsstGetVersionFunc>;

// AsstLog
typedef AsstLogFunc = void Function(StringPtr, StringPtr);
typedef AsstLogNative = NativeFunction<Void Function(StringPtr, StringPtr)>;

// AsstLoadResource
typedef AsstLoadResourceFunc = bool Function(StringPtr);
typedef AsstLoadResourceNative = NativeFunction<Bool Function(StringPtr)>;

typedef TaskId = int;
typedef TaskIdNative = Int32;
typedef StringPtr = Pointer<Utf8>;

typedef InitDartApiFunc = void Function(Pointer<Void>);
typedef InitDartApiNative = NativeFunction<Void Function(Pointer<Void>)>;


typedef CleanUpDartVMFunc = void Function();
typedef CleanUpDartVMNative = NativeFunction<Void Function()>;

typedef NativeFreeFunc = void Function(Pointer<Void>);
typedef NativeFreeNative = NativeFunction<Void Function(Pointer<Void>)>;

abstract class MaaCoreInterface {
  void destroy();
  bool connect(String adbPath, String address,[String config = '']);
  TaskId appendTask(String type, [Map<String, dynamic>? params]);
  bool setTaskParams(TaskId taskId, [Map<String, dynamic>? params]);
  bool start();
  bool stop();
  bool ctrlerClick({required int x, required int y, required block});
  void log(String logLevel, String msg);
  String getVersion();
}
