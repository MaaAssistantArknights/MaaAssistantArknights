import 'dart:async';
import 'dart:ffi';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import 'package:maa_core/typedefs.dart';
import 'package:path/path.dart' as p;

class MaaCore implements MaaCoreInterface {
  // Native Symbols
  // MaaCore
  static late AsstLoadResourceFunc _asstLoadResource;
  static late AsstCreateExFunc _asstCreateEx;
  static late AsstConnectFunc _asstConnect;
  static late AsstDestroyFunc _asstDestroy;
  static late AsstAppendTaskFunc _asstAppendTask;
  static late AsstSetTaskParamsFunc _asstSetTaskParams;
  static late AsstStartFunc _asstStart;
  static late AsstStopFunc _asstStop;
  static late AsstCtrlerClickFunc _asstCtrlerClick;
  static late AsstGetVersionFunc _asstGetVersion;
  static late AsstLogFunc _asstLog;

  // callbacklib
  static late InitDartApiFunc _initDartApi;
  static late Pointer<AsstCallbackNative> _asstCallback;
  static late NativeFreeFunc _nativeFree;

  static bool _resourceLoaded = false;
  static bool _apiInited = false;
  static bool _symbolsLoaded = false;

  late ReceivePort? _receivePort;
  late Assistant _asst;
  late StreamSubscription _portSubscription;
  late List<Pointer> _allocated;

  ReceivePort? get receivePort => _receivePort;
  static List<String> get _deps {
    if (Platform.isWindows) {
      return [
        'libiomp5md',
        'mklml',
        'mkldnn',
        'opencv_world453',
        'paddle_inference',
        'ppocr',
      ];
    }
    return [];
  }

  static String get MaaCoreLibName {
    if (Platform.isLinux) {
      return 'libMaaCore.so';
    } else if (Platform.isWindows) {
      return 'MaaCore.dll';
    }
    return 'libMaaCore.dylib';
  }

  static String get callbackLibName {
    if (Platform.isLinux) {
      return 'libCallback.so';
    } else if (Platform.isWindows) {
      return 'Callback.dll';
    }
    return 'libCallback.dylib';
  }

  static _loadResource(String dir) {
    print("init resource");
    final dirPtr = dir.toNativeUtf8();
    final res = _asstLoadResource(dirPtr);
    if (!res) {
      malloc.free(dirPtr);
      throw Exception("Failed to initialize MaaCore");
    }
    malloc.free(dirPtr);

    _resourceLoaded = true;
  }

  MaaCore(String libDir, [Function(String)? callback]) {
    init(libDir);

    _allocated = [];
    _receivePort = ReceivePort();
    final nativePort = _receivePort!.sendPort.nativePort;
    final portPtr = malloc.allocate<Int64>(sizeOf<Int64>());
    _asst = _asstCreateEx(_asstCallback, portPtr.cast<Void>());
    _allocated.add(portPtr);
    portPtr.value = nativePort;

    if (callback != null) {
      _portSubscription = _receivePort!.listen(_wrapCallback(callback));
    }
  }

  static void init(String libDir, {bool reloadResource = false}) {
    if (!_symbolsLoaded) {
      _loadNativeSymbols(libDir);
    }

    if (!_apiInited) {
      _initDartApi(NativeApi.initializeApiDLData);
      _apiInited = true;
    }

    if (!_resourceLoaded || reloadResource) {
      _loadResource(libDir);
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

  static void _loadNativeSymbols(String libDir) {
    _loadMaaCore(libDir);
    _loadCallbackLib(libDir);
    _symbolsLoaded = true;
  }

  static void _loadCallbackLib(String libDir) {
    final lib = DynamicLibrary.open(p.join(libDir, callbackLibName));
    _initDartApi = lib.lookup<InitDartApiNative>('init_dart_api').asFunction();
    _nativeFree = lib.lookup<NativeFreeNative>('native_free').asFunction();
    _asstCallback = lib.lookup<AsstCallbackNative>('callback');
  }

  static void _loadMaaCore(String libDir) {
    if (Platform.isWindows) {
      for (var dep in _deps) {
        print("load dep: $dep");
        DynamicLibrary.open(p.join(libDir, dep + '.dll'));
        print("loaded dep: $dep");
      }
    }
    final lib = DynamicLibrary.open(p.join(libDir, MaaCoreLibName));
    _asstLoadResource =
        lib.lookup<AsstLoadResourceNative>('AsstLoadResource').asFunction();
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

  static String get version {
    final ptr = _asstGetVersion();
    final str = ptr.toDartString();
    return str;
  }
}
