import time
import subprocess

class Bluestacks:
    @staticmethod
    def get_hyperv_port(conf_path=r"C:\ProgramData\BlueStacks_nxt\bluestacks.conf", instance_name="Pie64", read_imageinfo_from_config=False) -> int:
        """ 获取Hyper-v版蓝叠的adb port

        :param conf_path: bluestacks.conf 的路径+文件名
        :param instance_name: 多开的名称，在bluestacks.conf中以类似bst.instance.<instance_name>.status.adb_port的形式出现，如Nougat64，Pie64，Pie64_1等
        :return: adb端口
        """
        with open(conf_path, encoding="UTF-8") as f:
            configs = {
                line.split('=')[0].strip(): line.split('=')[1].strip().strip('"\n')
                for line in f
            }
            if read_imageinfo_from_config:
                instances = [i.strip('"') for i in configs['bst.installed_images'].split(',')]
                instance_name = instances[0]
        return int(configs[f'bst.instance.{instance_name}.status.adb_port'].replace('"', ""))
    
    @staticmethod
    def launch_emulator_win(emulator_path=r'C:\Program Files\BlueStacks_nxt\HD-Player.exe', post_delay=30, arg_instance=None):
        """ 启动模拟器
        如需管理员权限启动模拟器则以管理员权限执行Python脚本

        :param emulator_path: 模拟器可执行文件路径+文件名
        :param post_delay: 启动后的等待时间(s)
        :param arg_instance: 多开实例名，获取方式见get_hyperv_port注释。默认则置空。
        :return: 模拟器进程
        """
        args = [emulator_path]
        if arg_instance:
            args += ["--instance", arg_instance]
        emulator_proc = subprocess.Popen(args)
        time.sleep(post_delay)
        return emulator_proc
        
