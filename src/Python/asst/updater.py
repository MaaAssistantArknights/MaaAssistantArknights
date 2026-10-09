import json
import multiprocessing
import os
import platform
import re
import tarfile
import zipfile
from multiprocessing import Process, queues
from urllib import request
from urllib.error import HTTPError, URLError

from . import downloader
from .asst import Asst
from .utils import Version


class Updater:
    # API的地址
    Mirrors = ["https://api.maa.plus"]
    Summary_json = "/MaaAssistantArknights/api/version/summary.json"
    # 完整包会整体提供的资源目录，MaaCore 会递归加载其中的文件且不允许重名
    # 新版本删除或移动的文件若残留在这些目录中，会导致资源加载失败
    Package_managed_dirs = re.compile(
        r"^resource/(?:global/[^/]+/resource/)?(?:tasks|template)(?=/)"
    )

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
        p = Process(
            target=self._get_cur_version,
            args=(
                path,
                q,
            ),
        )
        p.start()
        p.join()
        # MAA当前版本 self.cur_version
        self.cur_version = q.get()

    @staticmethod
    def map_version_type(version):
        type_map = {
            Version.Nightly: "alpha",
            Version.Beta: "beta",
            Version.Stable: "stable",
        }
        return type_map.get(version, "stable")

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
                    "detail": "https://api.maa.plus/MaaAssistantArknights/api/version/alpha.json"
                  },
                  "beta": {
                    "version": "v4.24.0-beta.1",
                    "detail": "https://api.maa.plus/MaaAssistantArknights/api/version/beta.json"
                  },
                  "stable": {
                    "version": "v4.23.3",
                    "detail": "https://api.maa.plus/MaaAssistantArknights/api/version/stable.json"
                  }
                }
                """
                version_type = self.map_version_type(self.version)
                latest_version = response_data[version_type]["version"]
                version_detail = response_data[version_type]["detail"]
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
        if system == "Linux":
            machine = platform.machine()
            if machine == "aarch64":
                # Linux aarch64
                system_platform = "linux-aarch64"
            else:
                # Linux x86
                system_platform = "linux-x86_64"
        elif system == "Windows":
            machine = platform.machine()
            if machine == "AMD64" or machine == "x86_64":
                # Windows x86-64
                system_platform = "win-x64"
            else:
                # Windows ARM64
                system_platform = "win-arm64"
        # 请求的是https://api.maa.plus/MaaAssistantArknights/api/version/stable.json，或其他版本类型对应的url
        detail_json = request.urlopen(detail)
        detail_data = json.loads(detail_json.read().decode("utf-8"))
        assets_list = detail_data["details"]["assets"]  # 列表，子元素为字典
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
            assets_name = assets["name"]  # 示例值:MAA-v4.24.0-beta.1-win-arm64.zip
            # 正则匹配（用于选择当前系统及架构的版本）
            # 在线等一个不这么蠢的方法
            pattern = r"^MAA-.*-" + re.escape(system_platform) + r"\.(zip|tar\.gz)$"
            match = re.match(pattern, assets_name)
            if match:
                # Mirrors镜像列表
                mirrors = assets["mirrors"]
                github_url = assets["browser_download_url"]
                # 加上GitHub的release链接
                mirrors.append(github_url)
                return mirrors, assets_name
        return False, False

    @staticmethod
    def _remove_stale_resource_files(path, package_files):
        """
        删除完整包整体提供的资源目录中、新版本已不再包含的文件和空目录
        只处理 Package_managed_dirs 匹配且位于安装目录内的目录，用户自定义基建配置等其他文件不受影响
        返回被删除文件的相对路径列表
        """
        package_paths = set()
        case_variants = {}
        managed_dirs = set()
        for name in package_files:
            name = name.replace("\\", "/")
            while name.startswith("./"):
                name = name[2:]
            name = name.rstrip("/")
            if not name:
                continue
            package_paths.add(name)
            case_variants.setdefault(name.lower(), []).append(name)
            match = Updater.Package_managed_dirs.match(name)
            if match:
                managed_dirs.add(match.group(0))

        def in_package(rel_path, full_path):
            if rel_path in package_paths:
                return True
            # 大小写不敏感的文件系统上，仅大小写不同的路径就是新包中的同一个文件，不能删除
            for variant in case_variants.get(rel_path.lower(), []):
                try:
                    if os.path.samefile(
                        full_path, os.path.join(path, *variant.split("/"))
                    ):
                        return True
                except OSError:
                    pass
            return False

        errors = []

        def on_walk_error(e):
            errors.append(e)
            Updater.custom_print(f"扫描旧资源目录失败: {e}")

        real_path = os.path.realpath(path)
        removed = []
        for managed_dir in sorted(managed_dirs):
            root = os.path.join(path, *managed_dir.split("/"))
            # 资源目录经符号链接指向安装目录之外时跳过，避免误删其他数据
            try:
                inside = (
                    os.path.commonpath([real_path, os.path.realpath(root)]) == real_path
                )
            except ValueError:
                inside = False
            if not inside:
                Updater.custom_print(f"{managed_dir} 指向安装目录之外，跳过清理")
                continue
            for dirpath, _, filenames in os.walk(
                root, topdown=False, onerror=on_walk_error
            ):
                for filename in filenames:
                    full_path = os.path.join(dirpath, filename)
                    rel_path = os.path.relpath(full_path, path).replace(os.sep, "/")
                    if in_package(rel_path, full_path):
                        continue
                    try:
                        os.remove(full_path)
                        removed.append(rel_path)
                    except OSError as e:
                        errors.append(e)
                        Updater.custom_print(f"删除旧资源文件失败: {rel_path}, {e}")
                if dirpath == root:
                    continue
                rel_dir = os.path.relpath(dirpath, path).replace(os.sep, "/")
                try:
                    if not os.listdir(dirpath) and not in_package(rel_dir, dirpath):
                        os.rmdir(dirpath)
                except OSError as e:
                    errors.append(e)
                    Updater.custom_print(f"删除旧资源目录失败: {rel_dir}, {e}")
        if errors:
            Updater.custom_print(
                "部分旧资源未能清理，如遇资源加载失败，请删除 resource 目录后重新解压更新包"
            )
        return removed

    def update(self):
        """
        主函数
        """
        # 从dll获取MAA的版本
        current_version = self.cur_version
        # 从API获取最新版本
        # latest_version：版本号; version_detail：对应的json地址
        latest_version, version_detail = self.get_latest_version()
        if not latest_version:  # latest_version为False代表获取失败
            self.custom_print("获取版本信息失败")
        elif (
            current_version == latest_version
        ):  # 通过比较二者是否一致判断是否需要更新（摆烂
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
            download_finished = False
            max_retry = 3
            for retry_frequency in range(max_retry):
                try:
                    Updater.custom_print(
                        "开始下载"
                        + (
                            f"，第{retry_frequency}次尝试"
                            if retry_frequency > 1
                            else ""
                        )
                    )
                    # 调用downloader方法进行下载
                    download_finished = downloader.file_download(
                        download_url_list=url_list, download_path=file
                    )
                    break  # RNM怎么会有这么蠢的人忘了写break啊淦
                except (HTTPError, URLError) as e:
                    Updater.custom_print(e)

            if not download_finished:
                Updater.custom_print("下载异常，更新失败")
                return
            # 解压下载的文件，
            Updater.custom_print("开始安装更新，请不要关闭")
            file_extension = os.path.splitext(filename)[1]
            unzip = False
            package_files = []
            # 根据拓展名选择解压算法
            # .zip(Windows)/.tar.gz(Linux)
            if file_extension == ".zip":
                zfile = zipfile.ZipFile(file, "r")
                package_files = zfile.namelist()
                zfile.extractall(self.path)
                zfile.close()
                unzip = True
            # .tar.gz拓展名的情况（按照这个方式得到的拓展名是.gz，但是解压的是tar.gz
            elif file_extension == ".gz":
                tfile = tarfile.open(file, "r:gz")
                package_files = tfile.getnames()
                tfile.extractall(self.path)
                tfile.close()
                unzip = True
            if unzip:
                # 解压只会覆盖文件，需要清理新版本已删除或移动的旧资源文件
                removed = self._remove_stale_resource_files(self.path, package_files)
                if removed:
                    Updater.custom_print(f"已清理{len(removed)}个旧版本残留的资源文件")
            # 删除压缩包
            os.remove(file)
            if unzip:
                Updater.custom_print("更新完成")
            else:
                Updater.custom_print("更新未完成")
