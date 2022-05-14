import 'dart:async';
import 'dart:ffi';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import 'package:maa_core/typedefs.dart';
import 'package:path/path.dart' as p;

class MaaCore implements MaaCoreInterface {
  late Assistant _asst;
  late AsstLoadResourceFunc _asstLoadResource;
  late AsstCreateFunc _asstCreate;
  late NativeFreeFunc _nativeFree;
  late AsstCreateExFunc _asstCreateEx;
  late AsstConnectFunc _asstConnect;
  late AsstDestroyFunc _asstDestroy;
  late AsstAppendTaskFunc _asstAppendTask;
  late AsstSetTaskParamsFunc _asstSetTaskParams;
  late AsstStartFunc _asstStart;
  late AsstStopFunc _asstStop;
  late AsstCtrlerClickFunc _asstCtrlerClick;
  late AsstGetVersionFunc _asstGetVersion;
  late AsstLogFunc _asstLog;
  static bool _resourceLoaded = false;
  late ReceivePort? _receivePort;
  static bool _apiInited = false;
  ReceivePort? get receivePort => _receivePort;
  late StreamSubscription _portSubscription;
  late List<Pointer> _allocated;
  String get meoAssistantLibName {
    if (Platform.isLinux) {
      return 'libMeoAssistant.so';
    } else if (Platform.isWindows) {
      return 'MeoAssistant.dll';
    }
    return 'libMeoAssistant.dylib';
  }

  String get callbackLibName {
    if (Platform.isLinux) {
      return 'libCallback.so';
    } else if (Platform.isWindows) {
      return 'Callback.dll';
    }
    return 'libCallback.dylib';
  }
  static Future<String> get platformVersion async {
    return Future<String>(() => '0.0.1');
  }

  void _loadResource(String path) {
    final strPtr = toManagedString(path);
    final result = _asstLoadResource(strPtr);
    _resourceLoaded = _resourceLoaded || result;
  }
  MaaCore(String libDir, [Function(String)? callback]) {
    _loadMeoAssistant(libDir);
    if (!_apiInited) {
      final callbackLib = DynamicLibrary.open(p.join(libDir, callbackLibName));
      final InitDartApiFunc initNativeApi =
          callbackLib.lookup<InitDartApiNative>('init_dart_api').asFunction();
      initNativeApi(NativeApi.initializeApiDLData);
      _apiInited = true;
    }

    _allocated = [];
    _nativeFree = _loadNativeFree(libDir);
    if (!_resourceLoaded) {
      _loadResource(libDir);
    }
    _receivePort = ReceivePort();
    final nativePort = _receivePort!.sendPort.nativePort;
    final portPtr = malloc.allocate<Int64>(sizeOf<Int64>());
    _asst = _asstCreateEx(_loadNativeCallback(libDir), portPtr.cast<Void>());
    _allocated.add(portPtr);
    portPtr.value = nativePort;
    if (callback != null) {
      _portSubscription = _receivePort!.listen(_wrapCallback(callback));
    }
  }

  void Function(dynamic) _wrapCallback(void Function(String) cb) {
    return (dynamic data) {
      // c will manage the memory for the string
      final Pointer<Utf8> ptr = Pointer.fromAddress(data as int);
      String msg = ptr.toDartString();
      print("msg from ptr ($ptr): $msg before free");
      _nativeFree(ptr.cast<Void>());
      cb(msg);
      
    };
  }

  Pointer<AsstCallbackNative> _loadNativeCallback(String libDir) {
    return DynamicLibrary.open(p.join(libDir, callbackLibName))
        .lookup<AsstCallbackNative>('callback');
  }

  NativeFreeFunc _loadNativeFree(String libDir) {
    return DynamicLibrary.open(p.join(libDir, callbackLibName))
        .lookup<NativeFreeNative>('native_free').asFunction();
  }

  void _loadMeoAssistant(String libDir) {
    final lib = DynamicLibrary.open(p.join(libDir, meoAssistantLibName));
    _asstLoadResource =
        lib.lookup<AsstLoadResourceNative>('AsstLoadResource').asFunction();
    _asstCreate = lib.lookup<AsstCreateNative>('AsstCreate').asFunction();
    _asstCreateEx = lib.lookup<AsstCreateExNative>('AsstCreateEx').asFunction();
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
  bool connect(adbPath, address, [config = '']) {
    return _asstConnect(
      _asst,
      toManagedString(adbPath),
      toManagedString(address),
      toManagedString(config),
    );
  }

  @override
  void destroy() {
    if (_receivePort != null) {
      _portSubscription.cancel();
      _receivePort!.close();
    }
    while (_allocated.isNotEmpty) {
      final ptr = _allocated.removeLast();
      malloc.free(ptr);
    }
    _asstDestroy(_asst);
  }

  @override
  TaskId appendTask(String type, [Map<String, dynamic>? params]) {
    final json = jsonEncode(params ?? {});
    final taskId = _asstAppendTask(
      _asst,
      toManagedString(type),
      toManagedString(json),
    );
    return taskId;
  }

  @override
  bool setTaskParams(TaskId taskId, [Map<String, dynamic>? params]) {
    final json = jsonEncode(params);
    final result = _asstSetTaskParams(
      _asst,
      taskId,
      toManagedString(json),
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
    _asstLog(toManagedString(logLevel), toManagedString(msg));
  }

  Pointer<Utf8> toManagedString(String str) {
    final ptr = str.toNativeUtf8();
    _allocated.add(ptr);
    return ptr;
  }
}
