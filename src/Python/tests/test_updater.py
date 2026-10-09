import importlib.util
import os
import sys
import tarfile
import tempfile
import types
import unittest
import zipfile
from unittest.mock import patch

# updater 引用的 downloader 依赖 requests，此处测试不涉及下载，缺失时用空模块占位
if importlib.util.find_spec("requests") is None:
    sys.modules["requests"] = types.ModuleType("requests")

from asst.updater import Updater  # noqa: E402

# 新版本完整包，其中 DroneConfirm.png 从 Dorm 目录移到了 Drone 目录
NEW_PACKAGE = [
    "libMaaCore.so",
    "resource/custom_infrast/153_layout_3_times_a_day.json",
    "resource/global/YoStarJP/resource/tasks/tasks.json",
    "resource/global/YoStarJP/resource/template/Kept.png",
    "resource/tasks/tasks.json",
    "resource/template/InfrastPic/Drone/DroneConfirm.png",
]

# 旧版本安装目录中残留、新版本已不再提供的资源文件
STALE_FILES = [
    "resource/global/YoStarJP/resource/template/Removed.png",
    "resource/tasks/Removed.json",
    "resource/template/InfrastPic/Dorm/DroneConfirm.png",
]

# 不在完整包整体提供的资源目录中，不应被清理
USER_FILES = [
    "debug/asst.log",
    "resource/custom_infrast/my_plan.json",
]


def write_file(root, rel_path, content=b"maa"):
    full_path = os.path.join(root, *rel_path.split("/"))
    os.makedirs(os.path.dirname(full_path), exist_ok=True)
    with open(full_path, "wb") as f:
        f.write(content)


class RemoveStaleResourceFilesTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.install_dir = os.path.join(self.temp_dir.name, "maa")
        self.package_dir = os.path.join(self.temp_dir.name, "package")
        for rel_path in STALE_FILES + USER_FILES:
            write_file(self.install_dir, rel_path, b"old")
        for rel_path in NEW_PACKAGE:
            write_file(self.package_dir, rel_path, b"new")
        self.print_patch = patch.object(Updater, "custom_print")
        self.print_patch.start()

    def tearDown(self):
        self.print_patch.stop()
        self.temp_dir.cleanup()

    def exists(self, rel_path):
        return os.path.exists(os.path.join(self.install_dir, *rel_path.split("/")))

    def remove_stale(self, package_files):
        return sorted(
            Updater._remove_stale_resource_files(self.install_dir, package_files)
        )

    def assert_stale_files_removed(self, removed):
        self.assertEqual(removed, sorted(STALE_FILES))
        for rel_path in STALE_FILES:
            self.assertFalse(self.exists(rel_path), rel_path)
        for rel_path in NEW_PACKAGE + USER_FILES:
            self.assertTrue(self.exists(rel_path), rel_path)
        self.assertFalse(self.exists("resource/template/InfrastPic/Dorm"))
        self.assertTrue(self.exists("resource/template/InfrastPic/Drone"))

    def test_tar_package(self):
        archive = os.path.join(self.temp_dir.name, "MAA-linux-x86_64.tar.gz")
        # 与 CI 在安装目录内执行 tar czvf ... . 一致，成员名带 ./ 前缀
        with tarfile.open(archive, "w:gz") as tfile:
            tfile.add(self.package_dir, arcname=".")
        with tarfile.open(archive, "r:gz") as tfile:
            package_files = tfile.getnames()
            tfile.extractall(self.install_dir)
        self.assert_stale_files_removed(self.remove_stale(package_files))

    def test_zip_package(self):
        archive = os.path.join(self.temp_dir.name, "MAA-win-x64.zip")
        with zipfile.ZipFile(archive, "w") as zfile:
            for rel_path in NEW_PACKAGE:
                zfile.write(
                    os.path.join(self.package_dir, *rel_path.split("/")), rel_path
                )
        with zipfile.ZipFile(archive, "r") as zfile:
            package_files = zfile.namelist()
            zfile.extractall(self.install_dir)
        self.assert_stale_files_removed(self.remove_stale(package_files))

    def test_case_only_rename_is_kept(self):
        write_file(self.install_dir, "resource/template/Battle.PNG")
        removed = self.remove_stale(["resource/template/battle.png"])
        self.assertTrue(self.exists("resource/template/Battle.PNG"))
        self.assertIn("resource/template/InfrastPic/Dorm/DroneConfirm.png", removed)

    def test_package_without_managed_dirs_removes_nothing(self):
        removed = self.remove_stale(
            ["libMaaCore.so", "resource/custom_infrast/153_layout_3_times_a_day.json"]
        )
        self.assertEqual(removed, [])
        for rel_path in STALE_FILES + USER_FILES:
            self.assertTrue(self.exists(rel_path), rel_path)


if __name__ == "__main__":
    unittest.main()
