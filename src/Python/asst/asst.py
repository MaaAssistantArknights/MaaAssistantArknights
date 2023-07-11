import ctypes
import ctypes.util
import json
import os
import pathlib
import platform
from typing import Union, Optional

from .utils import InstanceOptionType, JSON


class Asst:
    CallBackType = ctypes.CFUNCTYPE(
        None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)
    """
    回调函数，使用实例可参照 my_callback

    :params:
        ``param1 message``: 消息类型
        ``param2 details``: json string
        ``param3 arg``:     自定义参数
    """

    @staticmethod
    def load(path: Union[pathlib.Path, str], incremental_path: Optional[Union[pathlib.Path, str]] = None,
             user_dir: Optional[Union[pathlib.Path, str]] = None) -> bool:
        """
        加载 dll 及资源

        :params:
            ``path``:    DLL及资源所在文件夹路径
            ``incremental_path``:   增量资源所在文件夹路径
            ``user_dir``:   用户数据（日志、调试图片等）写入文件夹路径
        """

        platform_values = {
            'windows': {
                'libpath': 'MaaCore.dll',
                'environ_var': 'PATH'
            },
            'darwin': {
                'libpath': 'libMaaCore.dylib',
                'environ_var': 'DYLD_LIBRARY_PATH'
            },
            'linux': {
                'libpath': 'libMaaCore.so',
                'environ_var': 'LD_LIBRARY_PATH'
            }
        }
        lib_import_func = None

        platform_type = platform.system().lower()
        if platform_type == 'windows':
            lib_import_func = ctypes.WinDLL
        else:
            lib_import_func = ctypes.CDLL

        Asst.__libpath = pathlib.Path(path) / platform_values[platform_type]['libpath']
        try:
            os.environ[platform_values[platform_type]['environ_var']] += os.pathsep + str(path)
        except KeyError:
            os.environ[platform_values[platform_type]['environ_var']] = os.pathsep + str(path)

        try:
            Asst.__lib = lib_import_func(str(Asst.__libpath))
        except OSError:
            Asst.__libpath = ctypes.util.find_library('MaaCore')
            Asst.__lib = lib_import_func(str(Asst.__libpath))

        Asst.__set_lib_properties()

        ret: bool = True
        if user_dir:
            ret &= Asst.__lib.AsstSetUserDir(str(user_dir).encode('utf-8'))

        ret &= Asst.__lib.AsstLoadResource(str(path).encode('utf-8'))
        if incremental_path:
            ret &= Asst.__lib.AsstLoadResource(
                str(incremental_path).encode('utf-8'))

        return ret

    def __init__(self, callback: CallBackType = None, arg=None):
        """
        :params:
            ``callback``:   回调函数
            ``arg``:        自定义参数
        """

        if callback:
            self.__ptr = Asst.__lib.AsstCreateEx(callback, arg)
        else:
            self.__ptr = Asst.__lib.AsstCreate()

    def __del__(self):
        Asst.__lib.AsstDestroy(self.__ptr)
        self.__ptr = None

    def set_instance_option(self, option_type: InstanceOptionType, option_value: str):
        """
        设置额外配置
        参见${MaaAssistantArknights}/src/MaaCore/Assistant.cpp#set_instance_option

        :params:
            ``externa_config``: 额外配置类型
            ``config_value``:   额外配置的值

        :return: 是否设置成功
        """
        return Asst.__lib.AsstSetInstanceOption(self.__ptr,
                                                int(option_type), option_value.encode('utf-8'))

    def connect(self, adb_path: str, address: str, config: str = 'General'):
        """
        连接设备

        :params:
            ``adb_path``:       adb 程序的路径
            ``address``:        adb 地址+端口
            ``config``:         adb 配置，可参考 resource/config.json

        :return: 是否连接成功
        """
        return Asst.__lib.AsstConnect(self.__ptr,
                                      adb_path.encode('utf-8'), address.encode('utf-8'), config.encode('utf-8'))

    TaskId = int

    def append_task(self, type_name: str, params: JSON = {}) -> TaskId:
        """
        添加任务

        :params:
            ``type_name``:  任务类型，请参考 docs/集成文档.md
            ``params``:     任务参数，请参考 docs/集成文档.md

        :return: 任务 ID, 可用于 set_task_params 接口
        """
        return Asst.__lib.AsstAppendTask(self.__ptr, type_name.encode('utf-8'),
                                         json.dumps(params, ensure_ascii=False).encode('utf-8'))

    def set_task_params(self, task_id: TaskId, params: JSON) -> bool:
        """
        动态设置任务参数

        :params:
            ``task_id``:  任务 ID, 使用 append_task 接口的返回值
            ``params``:   任务参数，同 append_task 接口，请参考 docs/集成文档.md

        :return: 是否成功
        """
        return Asst.__lib.AsstSetTaskParams(self.__ptr, task_id, json.dumps(params, ensure_ascii=False).encode('utf-8'))

    def start(self) -> bool:
        """
        开始任务

        :return: 是否成功
        """
        return Asst.__lib.AsstStart(self.__ptr)

    def stop(self) -> bool:
        """
        停止并清空所有任务

        :return: 是否成功
        """
        return Asst.__lib.AsstStop(self.__ptr)

    def running(self) -> bool:
        """
        是否正在运行

        :return: 是否正在运行
        """
        return Asst.__lib.AsstRunning(self.__ptr)

    @staticmethod
    def log(level: str, message: str) -> None:
        """
        打印日志

        :params:
            ``level``:      日志等级标签
            ``message``:    日志内容
        """

        Asst.__lib.AsstLog(level.encode('utf-8'), message.encode('utf-8'))

    def get_version(self) -> str:
        """
        获取DLL版本号

        : return: 版本号
        """
        return Asst.__lib.AsstGetVersion().decode('utf-8')

    @staticmethod
    def __set_lib_properties():
        Asst.__lib.AsstSetUserDir.restype = ctypes.c_bool
        Asst.__lib.AsstSetUserDir.argtypes = (
            ctypes.c_char_p,)

        Asst.__lib.AsstLoadResource.restype = ctypes.c_bool
        Asst.__lib.AsstLoadResource.argtypes = (
            ctypes.c_char_p,)

        Asst.__lib.AsstCreate.restype = ctypes.c_void_p
        Asst.__lib.AsstCreate.argtypes = ()

        Asst.__lib.AsstCreateEx.restype = ctypes.c_void_p
        Asst.__lib.AsstCreateEx.argtypes = (
            ctypes.c_void_p, ctypes.c_void_p,)

        Asst.__lib.AsstDestroy.argtypes = (ctypes.c_void_p,)

        Asst.__lib.AsstSetInstanceOption.restype = ctypes.c_bool
        Asst.__lib.AsstSetInstanceOption.argtypes = (
            ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p,)

        Asst.__lib.AsstConnect.restype = ctypes.c_bool
        Asst.__lib.AsstConnect.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,)

        Asst.__lib.AsstAppendTask.restype = ctypes.c_int
        Asst.__lib.AsstAppendTask.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p)

        Asst.__lib.AsstSetTaskParams.restype = ctypes.c_bool
        Asst.__lib.AsstSetTaskParams.argtypes = (
            ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p)

        Asst.__lib.AsstStart.restype = ctypes.c_bool
        Asst.__lib.AsstStart.argtypes = (ctypes.c_void_p,)

        Asst.__lib.AsstStop.restype = ctypes.c_bool
        Asst.__lib.AsstStop.argtypes = (ctypes.c_void_p,)

        Asst.__lib.AsstRunning.restype = ctypes.c_bool
        Asst.__lib.AsstRunning.argtypes = (ctypes.c_void_p,)

        Asst.__lib.AsstGetVersion.restype = ctypes.c_char_p

        Asst.__lib.AsstLog.restype = None
        Asst.__lib.AsstLog.argtypes = (
            ctypes.c_char_p, ctypes.c_char_p)
