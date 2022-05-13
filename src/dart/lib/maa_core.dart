import 'dart:ffi';
import 'dart:convert';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import 'package:maa_core/typedefs.dart';
import 'package:path/path.dart' as p;

class MaaCore implements MaaCoreInterface {
  late Assistant _asst;
  late AsstConnectFunc _asstConnect;
  late AsstDestroyFunc _asstDestroy;
  late AsstAppendTaskFunc _asstAppendTask;
  late AsstSetTaskParamsFunc _asstSetTaskParams;
  late AsstStartFunc _asstStart;
  late AsstStopFunc _asstStop;
  late AsstCtrlerClickFunc _asstCtrlerClick;
  late AsstGetVersionFunc _asstGetVersion;
  late AsstLogFunc _asstLog;
  static bool resourceLoaded = false;
  late ReceivePort? _receivePort;
  ReceivePort? get receivePort => _receivePort;
  late Pointer<Int64> _pointer;
  static Future<String> get platformVersion async {
    return Future<String>(() => '0.0.1');
  }

  static bool loadResource(DynamicLibrary lib, String resourceDir) {
    final AsstLoadResourceFunc asstLoadResource =
        lib.lookup<AsstLoadResourceNative>('AsstLoadResource').asFunction();
    if (!resourceLoaded) {
      return asstLoadResource(resourceDir.toNativeUtf8());
    }
    return true;
  }

  MaaCore(String libDir, [Function(dynamic)? callback]) {
  
    _loadCppLib(libDir);
    final _createWithDartPort = loadAsstCreateDart(libDir);
    _receivePort = ReceivePort();
    final portPtr = malloc.allocate<Int64>(sizeOf<Int64>());
    portPtr.value = _receivePort!.sendPort.nativePort;
    _asst = _createWithDartPort(portPtr);
    if (callback != null) {
      _receivePort!.listen(callback);
    }
  }

  void _loadCppLib(String libDir) {
    final lib = DynamicLibrary.open(p.join(libDir, 'libMeoAssistant.so'));
   
    _asstConnect = lib.lookup<AsstConnectNative>('AsstConnect').asFunction();
    _asstDestroy = lib.lookup<AsstDestroyNative>('AsstDestroy').asFunction();
    _asstAppendTask =
        lib.lookup<AsstAppendTaskNative>('AsstAppendTask').asFunction();
    _asstSetTaskParams =
        lib.lookup<AsstSetTaskParamsNative>('AsstSetTaskParams').asFunction();
    _asstStart = lib.lookup<AsstStartNative>('AsstStart').asFunction();
    _asstStop = lib.lookup<AsstStopNative>('AsstStop').asFunction();
    _asstCtrlerClick =
        lib.lookup<AsstCtrlerClickNative>('AsstCtrlerClick').asFunction();
    _asstGetVersion =
        lib.lookup<AsstGetVersionNative>('AsstGetVersion').asFunction();
    _asstLog = lib.lookup<AsstLogNative>('AsstLog').asFunction();
  }

  @override
  bool connect({
    required String adbPath,
    required String address,
    String config = '',
  }) {
    return _asstConnect(
      _asst,
      adbPath.toNativeUtf8(),
      address.toNativeUtf8(),
      config.toNativeUtf8(),
    );
  }

  @override
  void destroy() {
    if (_receivePort != null) {
      _receivePort!.close();
      malloc.free(_pointer);
    }
    _asstDestroy(_asst);
  }

  @override
  TaskId appendTask(String type, [Map<String, dynamic>? params]) {
    final json = jsonEncode(params ?? {});
    final taskId = _asstAppendTask(
      _asst,
      type.toNativeUtf8(),
      json.toNativeUtf8(),
    );
    return taskId;
  }

  @override
  bool setTaskParams(TaskId taskId, [Map<String, dynamic>? params]) {
    final json = jsonEncode(params);
    final result = _asstSetTaskParams(
      _asst,
      taskId,
      json.toNativeUtf8(),
    );
    return result;
  }

  @override
  bool start() {
    return _asstStart(_asst);
  }

  @override
  bool stop() {
    return _asstStop(_asst);
  }

  @override
  bool ctrlerClick({required int x, required int y, required block}) {
    return _asstCtrlerClick(_asst, x, y, block);
  }

  @override
  String getVersion() {
    return _asstGetVersion().toDartString();
  }

  @override
  void log(String logLevel, String msg) {
    _asstLog(logLevel.toNativeUtf8(), msg.toNativeUtf8());
  }
}

AsstCreateDartFunc loadAsstCreateDart(String libPath) {
  final lib = DynamicLibrary.open(p.join(libPath, 'libMeoAssistantDart.so'));
  return lib
      .lookup<AsstCreateDartNative>('AsstCreateWithDartPort')
      .asFunction();
}

InitDartVMFunc loadInitDartVM(String libPath) {
  final lib = DynamicLibrary.open(p.join(libPath, 'libMeoAssistantDart.so'));
  return lib.lookup<InitDartVMNative>('InitDartVM').asFunction();
}

CleanUpDartVMFunc loadCleanUpDartVM(String libPath) {
  final lib = DynamicLibrary.open(p.join(libPath, 'libMeoAssistantDart.so'));
  return lib.lookup<CleanUpDartVMNative>('CleanUpDartVM').asFunction();
}
