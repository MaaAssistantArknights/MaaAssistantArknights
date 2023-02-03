import os
import json
import ctypes
import pathlib
import zipfile
import platform
import multiprocessing

from shutil import rmtree
from urllib import request
from urllib.error import *
from urllib.request import Request
from multiprocessing import queues
from multiprocessing import Process
from enum import Enum, IntEnum, unique, auto
from typing import Union, Dict, List, Any, Type, Optional

JSON = Union[Dict[str, Any], List[Any], int, str, float, bool, Type[None]]


class InstanceOptionType(IntEnum):
    touch_type = 2
    deployment_with_pause = 3


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
            # 手动加载onnxruntime.dll以避免部分版本的python错误地从System32加载旧版本
            try:
                lib_import_func(str(pathlib.Path(path) / 'onnxruntime.dll'))
            except Exception as e:
                print(e)
                pass
        else:
            lib_import_func = ctypes.CDLL

        Asst.__libpath = pathlib.Path(path) / platform_values[platform_type]['libpath']
        try:
            os.environ[platform_values[platform_type]['environ_var']] += os.pathsep + str(path)
        except KeyError:
            os.environ[platform_values[platform_type]['environ_var']] = os.pathsep + str(path)
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


@unique
class Message(Enum):
    """
    回调消息

    请参考 docs/回调消息.md
    """
    InternalError = 0

    InitFailed = auto()

    ConnectionInfo = auto()

    AllTasksCompleted = auto()

    TaskChainError = 10000

    TaskChainStart = auto()

    TaskChainCompleted = auto()

    TaskChainExtraInfo = auto()

    TaskChainStopped = auto()

    SubTaskError = 20000

    SubTaskStart = auto()

    SubTaskCompleted = auto()

    SubTaskExtraInfo = auto()

    SubTaskStopped = auto()


@unique
class Version(Enum):
    """
    目标版本
    """
    Nightly = auto()

    Beta = auto()

    Stable = auto()


class Updater:
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) '
                      'AppleWebKit/537.36 (KHTML, like Gecko) '
                      'Chrome/97.0.4692.99 '
                      'Safari/537.36 '
                      'Edg/97.0.1072.76'
    }
    RequestUrl = "repos/MaaAssistantArknights/MaaRelease/releases"
    StableRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/latest"
    MaaReleaseRequestUrlByTag = "repos/MaaAssistantArknights/MaaRelease/releases/tags/"
    InfoRequestUrl = "repos/MaaAssistantArknights/MaaAssistantArknights/releases/tags/"

    @staticmethod
    def custom_print(s):
        """
        可以被monkey patch的print，在其他GUI上使用可以被替换为任何需要的输出
        """
        print(s)

    @staticmethod
    def _get_cur_version(path, q):
        """
        获取当前版本
        """
        Asst.load(path=path)
        q.put(Asst().get_version())

    def __init__(self, path, version):
        self.path = path
        self.version = version
        self.latest_json = None
        self.latest_version = None
        self.assets_object = None

        # 使用子线程获取当前版本后关闭，避免占用dll
        q = queues.Queue(1, ctx=multiprocessing)
        p = Process(target=self._get_cur_version, args=(path, q,))
        p.start()
        p.join()
        self.cur_version = q.get()

    @staticmethod
    def _request_github_api(url, retry):
        request_resource = ["https://api.github.com/", "https://api.kgithub.com/"]
        for _ in range(retry):
            for resource in request_resource:
                try:
                    response = request.urlopen(Request(url=resource + url, headers=Updater.headers), timeout=20)
                    data = response.read().decode('utf-8')
                    Updater.custom_print(f'访问成功，URL: {resource + url}')
                    return data
                except (HTTPError, URLError) as e:
                    Updater.custom_print(f'访问成功，URL: {resource + url}')
                    Updater.custom_print(e)
                    if _ == retry - 1:
                        raise

    def _is_nightly_version(self, ver: Union[str, None]):
        if ver is None:
            ver = self.cur_version

        if '-' not in ver:
            return False
        last_id = ver.split('.')[-1]
        return last_id.startswith('g') and len(last_id) >= 7

    def _is_std_version(self, ver: Union[str, None]):
        if ver is None:
            ver = self.cur_version

        if ver == 'DEBUG VERSION':
            return False
        elif ver.startswith('c') or ver.startswith('20') or 'local' in ver:
            return False
        elif self._is_nightly_version(ver):
            return False
        return True

    @staticmethod
    def _split_version(ver: str):
        if '-' in ver:
            pre, sub = ver.split('-', 1)
        else:
            pre = ver
            sub = None
        pre = pre.split('.')
        pre[0] = pre[0][1:]
        if sub:
            sub = sub.split('.')
            sub = sub[:-1]
            if sub[0] == 'alpha':
                sub[0] = '1'
            elif sub[0] == 'beta':
                sub[0] = '2'
            elif sub[0] == 'rc':
                sub[0] = '3'
            out = pre + sub
        else:
            out = pre
        for i in range(len(out)):
            out[i] = int(out[i])
        return out

    @staticmethod
    def _compare_version(a, b):
        a = Updater._split_version(a)
        b = Updater._split_version(b)
        for i in range(len(a)):
            try:
                if a[i] < b[i]:
                    return True
                elif a[i] > b[i]:
                    return False
            except IndexError:
                return False
        if len(b) > len(a):
            return True
        return False

    def _check_update(self):
        max_retry = 2
        if self.version == Version.Stable:
            stable_response = self._request_github_api(self.StableRequestUrl, max_retry)
            if len(stable_response) == 0:
                raise HTTPError

            self.latest_json = json.loads(stable_response)
            self.latest_version = self.latest_json['tag_name']

            stable_response = self._request_github_api(self.MaaReleaseRequestUrlByTag + self.latest_version, max_retry)
            if len(stable_response) == 0:
                return False
            self.latest_json = json.loads(stable_response)
        else:
            response = self._request_github_api(self.RequestUrl, max_retry)
            if len(response) == 0:
                raise HTTPError
            release_array = json.loads(response)
            self.latest_json = None
            for item in release_array:
                if not self.version == Version.Nightly and not self._is_std_version(item['tag_name']):
                    continue
                self.latest_json = item
                break

        if self.latest_json is None:
            return False
        self.latest_version = self.latest_json['tag_name']
        release_assets = self.latest_json['assets']

        if self.version == Version.Nightly and self.cur_version == self.latest_version:
            return False
        elif not self._compare_version(self.cur_version, self.latest_version):
            return False

        if self._is_std_version(self.latest_version):
            info_responce = self._request_github_api(self.InfoRequestUrl + self.latest_version, max_retry)
            if len(info_responce) == 0:
                raise HTTPError
            self.latest_json = json.loads(info_responce)

        for cur_assets in release_assets:
            name = cur_assets['name'].lower()
            if 'ota' in name and 'win' in name and f'{self.cur_version}_{self.latest_version}' in name:
                self.assets_object = cur_assets
                break
        return True

    def _remove_file(self):
        def remove_with_print(s):
            self.custom_print(f'删除文件：{s}')
            os.remove(s)

        removelist_path = os.path.join(self.path, 'removelist.txt')
        if os.path.isfile(removelist_path):
            with open(removelist_path, 'r') as f:
                file_list = f.readlines()
                for file in file_list:
                    file_path = os.path.join(self.path, file)
                    if os.path.isfile(file_path):
                        remove_with_print(file_path)
                    elif os.path.isdir(file_path):
                        self.custom_print(f'删除文件夹：{file_path}')
                        rmtree(file_path)
                f.close()
            remove_with_print(removelist_path)

        md5sum_path = os.path.join(self.path, 'md5sum.txt')
        if os.path.isfile(md5sum_path):
            remove_with_print(md5sum_path)
        for file in os.listdir(self.path):
            if 'OTA' in file:
                file_path = os.path.join(self.path, file)
                remove_with_print(file_path)

    def update(self):
        """
        更新主函数
        """
        max_retry = 3
        if not self._check_update():
            Updater.custom_print('目前不需要更新')
            return False

        # 下载
        replace_list = [
            ('github.com', 'ota.maa.plus'),
            ('github.com', 'download.fastgit.org')
        ]
        for i in range(max_retry):
            url = self.assets_object['browser_download_url']
            file = os.path.join(self.path, url.split('/')[-1])
            if i < 2:
                url = url.replace(replace_list[i][0], replace_list[i][1])
                try:
                    Updater.custom_print(f'开始下载更新包，URL：{url}')
                    request.urlretrieve(url, file)
                    break
                except (HTTPError, URLError) as e:
                    Updater.custom_print(e)

        # 解压
        Updater.custom_print('开始安装更新，请不要关闭')
        zfile = zipfile.ZipFile(file, 'r')
        zfile.extractall(self.path)
        zfile.close()

        # 删除
        self._remove_file()

        Updater.custom_print('更新完成')
