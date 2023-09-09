import json
import multiprocessing
import platform
import re
import os
import tarfile
import zipfile
from multiprocessing import queues, Process
from urllib import request
from urllib.error import HTTPError, URLError

from .asst import Asst
from .utils import Version
from . import downloader


class Updater:
    # API的地址
    Mirrors = ["https://ota.maa.plus"]
    Summary_json = "/MaaAssistantArknights/api/version/summary.json"

    @staticmethod
    def custom_print(s):
        """
        可以被monkey patch的print，在其他GUI上使用可以被替换为任何需要的输出
        """
        print(s)

    @staticmethod
    def _get_cur_version(path, q):
        """
        从MaaCore.dll获取当前版本号
        这里是复用原来的方法
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
        # MAA当前版本 self.cur_version
        self.cur_version = q.get()

    @staticmethod
    def map_version_type(version):
        type_map = {
            Version.Nightly: 'alpha',
            Version.Beta: 'beta',
            Version.Stable: 'stable'
        }
        return type_map.get(version, 'stable')

    def get_latest_version(self):
        """
        从API获取最新版本
        """
        api_url = self.Mirrors
        version_summary = self.Summary_json
        retry = 3
        for retry_times in range(retry):
            # 在重试次数限制内依次请求每一个镜像
            i = retry_times % len(api_url)
            request_url = api_url[i] + version_summary
            try:
                response_json = request.urlopen(request_url)
                response_data = json.loads(response_json.read().decode("utf-8"))
                """
                解析JSON
                e.g.
                {
                  "alpha": {
                    "version": "v4.24.0-beta.1.d006.g27dee653d",
                    "detail": "https://ota.maa.plus/MaaAssistantArknights/api/version/alpha.json"
                  },
                  "beta": {
                    "version": "v4.24.0-beta.1",
                    "detail": "https://ota.maa.plus/MaaAssistantArknights/api/version/beta.json"
                  },
                  "stable": {
                    "version": "v4.23.3",
                    "detail": "https://ota.maa.plus/MaaAssistantArknights/api/version/stable.json"
                  }
                }
                """
                version_type = self.map_version_type(self.version)
                latest_version = response_data[version_type]['version']
                version_detail = response_data[version_type]['detail']
                return latest_version, version_detail
            except Exception as e:
                self.custom_print(e)
                continue
        return False, False

    @staticmethod
    def get_download_url(detail):
        """
        1.获取系统及架构信息
        2.找到对应的版本
        3.返回镜像url列表&文件名
        """
        """
        获取系统信息，包括：
            架构：ARM、x86
            系统：Linux、Windows
        默认Windows x86_64
        """
        system_platform = "win-x64"
        system = platform.system()
        if system == 'Linux':
            machine = platform.machine()
            if machine == 'aarch64':
                # Linux aarch64
                system_platform = "linux-aarch64"
            else:
                # Linux x86
                system_platform = "linux-x86_64"
        elif system == 'Windows':
            machine = platform.machine()
            if machine == 'AMD64' or machine == 'x86_64':
                # Windows x86-64
                system_platform = "win-x64"
            else:
                # Windows ARM64
                system_platform = "win-arm64"
        # 请求的是https://ota.maa.plus/MaaAssistantArknights/api/version/stable.json，或其他版本类型对应的url
        detail_json = request.urlopen(detail)
        detail_data = json.loads(detail_json.read().decode("utf-8"))
        assets_list = detail_data["details"]["assets"]     # 列表，子元素为字典
        # 找到对应系统和架构的版本
        for assets in assets_list:
            """
            结构示例
            assets:
            {
                "name": "MAA-v4.24.0-beta.1.d006.g27dee653d-win-x64.zip",
                "size": 145677836,
                "browser_download_url": "https://github.com/MaaAssistantArknights/MaaRelease/releases/download/v4.24.0-beta.1.d006.g27dee653d/MAA-v4.24.0-beta.1.d006.g27dee653d-win-x64.zip",
                "mirrors": [
                  "https://s3.maa-org.net:25240/maa-release/MaaAssistantArknights/MaaRelease/releases/download/v4.24.0-beta.1.d006.g27dee653d/MAA-v4.24.0-beta.1.d006.g27dee653d-win-x64.zip",
                  "https://agent.imgg.dev/MaaAssistantArknights/MaaRelease/releases/download/v4.24.0-beta.1.d006.g27dee653d/MAA-v4.24.0-beta.1.d006.g27dee653d-win-x64.zip",
                  "https://maa.r2.imgg.dev/MaaAssistantArknights/MaaRelease/releases/download/v4.24.0-beta.1.d006.g27dee653d/MAA-v4.24.0-beta.1.d006.g27dee653d-win-x64.zip"
                ]
            }
            """
            assets_name = assets["name"]        # 示例值:MAA-v4.24.0-beta.1-win-arm64.zip
            # 正则匹配（用于选择当前系统及架构的版本）
            # 在线等一个不这么蠢的方法
            pattern = r"^MAA-.*" + re.escape(system_platform)
            match = re.match(pattern, assets_name)
            if match:
                # Mirrors镜像列表
                mirrors = assets["mirrors"]
                github_url = assets["browser_download_url"]
                # 加上GitHub的release链接
                mirrors.append(github_url)
                return mirrors, assets_name
        return False, False

    def update(self):
        """
        主函数
        """
        # 从dll获取MAA的版本
        current_version = self.cur_version
        # 从API获取最新版本
        # latest_version：版本号; version_detail：对应的json地址
        latest_version, version_detail = self.get_latest_version()
        if not latest_version:                      # latest_version为False代表获取失败
            self.custom_print("获取版本信息失败")
        elif current_version == latest_version:     # 通过比较二者是否一致判断是否需要更新（摆烂
            self.custom_print("当前为最新版本，无需更新")
        else:
            self.custom_print(f"检测到最新版本:{latest_version}，正在更新")
            # 开始更新逻辑
            # 解析version_detail的JSON信息
            # 通过API获取下载地址列表和对应文件名
            url_list, filename = self.get_download_url(version_detail)
            if not url_list:
                # 如果请求失败则返回False
                # （此返回值可能会在非Windows-x86_64的程序更新alpha版时出现）
                self.custom_print("未找到适用于当前系统的更新包")
                # 直接结束
                return
            # 将路径和文件名拼合成绝对路径
            # 默认在maa主程序/MaaCore.dll所在路径下
            file = os.path.join(self.path, filename)
            # 下载，调用Downloader下载器，使用url_list（镜像url列表）和file（文件保存路径）两个参数
            # Proxy参数没加，因为可能有问题（也可能没问题反正我晚上Clash连不上）
            # 重试3次
            max_retry = 3
            for retry_frequency in range(max_retry):
                try:
                    Updater.custom_print("开始下载" + (f"，第{retry_frequency}次尝试" if retry_frequency > 1 else ""))
                    # 调用downloader方法进行下载
                    downloader.file_download(download_url_list=url_list, download_path=file)
                    break           # RNM怎么会有这么蠢的人忘了写break啊淦
                except(HTTPError, URLError) as e:
                    Updater.custom_print(e)

            # 解压下载的文件，
            Updater.custom_print('开始安装更新，请不要关闭')
            file_extension = os.path.splitext(filename)[1]
            unzip = False
            # 根据拓展名选择解压算法
            # .zip(Windows)/.tar.gz(Linux)
            if file_extension == '.zip':
                zfile = zipfile.ZipFile(file, 'r')
                zfile.extractall(self.path)
                zfile.close()
                unzip = True
            # .tar.gz拓展名的情况（按照这个方式得到的拓展名是.gz，但是解压的是tar.gz
            elif file_extension == '.gz':
                tfile = tarfile.open(file, 'r:gz')
                tfile.extractall(self.path)
                tfile.close()
                unzip = True
            # 删除压缩包
            os.remove(file)
            if unzip:
                Updater.custom_print('更新完成')
            else:
                Updater.custom_print('更新未完成')
