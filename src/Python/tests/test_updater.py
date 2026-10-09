import importlib.util
import os
import shutil
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
from asst.utils import Version  # noqa: E402

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
        self.print_mock = self.print_patch.start()

    def tearDown(self):
        self.print_patch.stop()
        self.temp_dir.cleanup()

    def path_of(self, rel_path):
        return os.path.join(self.install_dir, *rel_path.split("/"))

    def exists(self, rel_path):
        return os.path.exists(self.path_of(rel_path))

    def printed(self):
        return "\n".join(str(c.args[0]) for c in self.print_mock.call_args_list)

    def remove_stale(self, package_files):
        return sorted(
            Updater._remove_stale_resource_files(self.install_dir, package_files)
        )

    def make_tar(self):
        archive = os.path.join(self.temp_dir.name, "MAA-v2-linux-x86_64.tar.gz")
        # 与 CI 在安装目录内执行 tar czvf ... . 一致，成员名带 ./ 前缀
        with tarfile.open(archive, "w:gz") as tfile:
            tfile.add(self.package_dir, arcname=".")
        return archive

    def assert_stale_files_removed(self):
        for rel_path in STALE_FILES:
            self.assertFalse(self.exists(rel_path), rel_path)
        for rel_path in NEW_PACKAGE + USER_FILES:
            self.assertTrue(self.exists(rel_path), rel_path)
        self.assertFalse(self.exists("resource/template/InfrastPic/Dorm"))
        self.assertTrue(self.exists("resource/template/InfrastPic/Drone"))

    def test_tar_package(self):
        with tarfile.open(self.make_tar(), "r:gz") as tfile:
            package_files = tfile.getnames()
            tfile.extractall(self.install_dir)
        self.assertEqual(self.remove_stale(package_files), sorted(STALE_FILES))
        self.assert_stale_files_removed()

    def test_zip_package(self):
        archive = os.path.join(self.temp_dir.name, "MAA-v2-win-x64.zip")
        with zipfile.ZipFile(archive, "w") as zfile:
            for rel_path in NEW_PACKAGE:
                zfile.write(
                    os.path.join(self.package_dir, *rel_path.split("/")), rel_path
                )
        with zipfile.ZipFile(archive, "r") as zfile:
            package_files = zfile.namelist()
            zfile.extractall(self.install_dir)
        self.assertEqual(self.remove_stale(package_files), sorted(STALE_FILES))
        self.assert_stale_files_removed()

    def test_case_only_rename(self):
        old_rel = "resource/tasks/Roguelike/Base.json"
        new_rel = "resource/tasks/roguelike/base.json"
        write_file(self.install_dir, old_rel, b"old")
        # 解压新包：大小写不敏感的文件系统上会写回同一个文件
        write_file(self.install_dir, new_rel, b"new")
        case_sensitive = not os.path.samefile(
            self.path_of(old_rel), self.path_of(new_rel)
        )

        removed = self.remove_stale([new_rel])

        with open(self.path_of(new_rel), "rb") as f:
            self.assertEqual(f.read(), b"new")
        if case_sensitive:
            # 新旧文件并存，旧文件会导致任务重名，连同清空的旧目录一起删除
            self.assertIn(old_rel, removed)
            self.assertNotIn("Roguelike", os.listdir(self.path_of("resource/tasks")))
        else:
            # 新旧路径是同一个文件，必须保留
            self.assertFalse(
                [p for p in removed if p.lower() == new_rel.lower()], removed
            )

    def test_symlinked_dir_outside_install_is_skipped(self):
        outside = os.path.join(self.temp_dir.name, "outside")
        write_file(outside, "Unrelated.png")
        template = self.path_of("resource/template")
        shutil.rmtree(template)
        try:
            os.symlink(outside, template, target_is_directory=True)
        except (OSError, NotImplementedError) as e:
            self.skipTest(f"cannot create symlink: {e}")

        self.assertEqual(self.remove_stale(["resource/template/New.png"]), [])
        self.assertTrue(os.path.exists(os.path.join(outside, "Unrelated.png")))

    @unittest.skipIf(
        os.name == "nt" or (hasattr(os, "geteuid") and os.geteuid() == 0),
        "requires POSIX permissions as a non-root user",
    )
    def test_scan_error_is_reported(self):
        locked = self.path_of("resource/template/InfrastPic")
        os.chmod(locked, 0)
        try:
            self.remove_stale(["resource/template/InfrastPic/Drone/DroneConfirm.png"])
        finally:
            os.chmod(locked, 0o755)
        self.assertIn("扫描旧资源目录失败", self.printed())
        self.assertIn("请删除 resource 目录", self.printed())

    def test_package_without_managed_dirs_removes_nothing(self):
        removed = self.remove_stale(
            ["libMaaCore.so", "resource/custom_infrast/153_layout_3_times_a_day.json"]
        )
        self.assertEqual(removed, [])
        for rel_path in STALE_FILES + USER_FILES:
            self.assertTrue(self.exists(rel_path), rel_path)

    def test_update_installs_package_and_removes_stale_files(self):
        archive = self.make_tar()

        def fake_download(download_url_list, download_path):
            shutil.copyfile(archive, download_path)
            return True

        # 跳过 __init__，其中会加载 MaaCore 获取当前版本
        updater = Updater.__new__(Updater)
        updater.path = self.install_dir
        updater.version = Version.Stable
        updater.cur_version = "v1"
        package_name = os.path.basename(archive)
        with patch.multiple(
            Updater,
            get_latest_version=lambda self: ("v2", ""),
            get_download_url=staticmethod(lambda detail: (["mirror"], package_name)),
        ):
            with patch(
                "asst.updater.downloader.file_download", side_effect=fake_download
            ):
                updater.update()

        self.assert_stale_files_removed()
        self.assertFalse(self.exists(package_name))
        self.assertIn(f"已清理{len(STALE_FILES)}个旧版本残留的资源文件", self.printed())
        self.assertIn("更新完成", self.printed())


if __name__ == "__main__":
    unittest.main()
