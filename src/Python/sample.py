import json
import pathlib
import time

from asst.asst import Asst
from asst.utils import Message, Version, InstanceOptionType
from asst.updater import Updater
from asst.emulator import Bluestacks


@Asst.CallBackType
def my_callback(msg, details, arg):
    m = Message(msg)
    d = json.loads(details.decode('utf-8'))

    print(m, d, arg)


if __name__ == "__main__":

    # 请设置为存放 dll 文件及资源的路径
    path = pathlib.Path(__file__).resolve().parent.parent

    # 设置更新器的路径和目标版本并更新
    Updater(path, Version.Stable).update()

    # 加载主资源及增量副资源
    #
    # 参数说明：
    # - path：主资源的路径。
    # - incremental_path：增量副资源的路径。副资源需要在主资源加载后再加载，以下是两种常见的使用场景：
    #   1. 加载外服增量资源：
    #      传入外服增量资源路径，先加载主资源，再加载增量副资源：
    #      Asst.load(path=path)  # 加载主资源
    #      Asst.load(path=path, incremental_path=path / 'resource' / 'global' / 'YoStarEN')  # 加载增量副资源
    #   2. 加载活动关卡资源（需先下载）：
    #      首先下载活动关卡的任务数据：
    #      import urllib.request
    #      ota_tasks_url = 'https://ota.maa.plus/MaaAssistantArknights/api/resource/tasks.json'
    #      ota_tasks_path = path / 'cache' / 'resource' / 'tasks.json'
    #      ota_tasks_path.parent.mkdir(parents=True, exist_ok=True)
    #      with open(ota_tasks_path, 'w', encoding='utf-8') as f:
    #          with urllib.request.urlopen(ota_tasks_url) as u:
    #              f.write(u.read().decode('utf-8'))
    #      然后加载主资源和增量副资源：
    #      Asst.load(path=path)  # 加载主资源
    #      Asst.load(path=path, incremental_path=path / 'cache')  # 加载下载的增量副资源
    #
    # 示例调用：
    # 1. 加载主资源：
    Asst.load(path=path)
    # 2. 加载增量副资源：
    # Asst.load(path=path, incremental_path=path / 'resource' / 'global' / 'YoStarEN')

    # 若需要获取详细执行信息，请传入 callback 参数
    # 例如 asst = Asst(callback=my_callback)
    asst = Asst()

    # 设置额外配置
    # 触控方案配置
    asst.set_instance_option(InstanceOptionType.touch_type, 'maatouch')
    # 暂停下干员
    # asst.set_instance_option(InstanceOptionType.deployment_with_pause, '1')

    # 启动模拟器。例如启动蓝叠模拟器的多开Pie64_1，并等待30s
    # Bluestacks.launch_emulator_win(r'C:\Program Files\BlueStacks_nxt\HD-Player.exe', 30, "Pie64_1")

    # 获取Hyper-v蓝叠的adb port
    # port = Bluestacks.get_hyperv_port(r"C:\ProgramData\BlueStacks_nxt\bluestacks.conf", "Pie64_1")

    # 请自行配置 adb 环境变量，或修改为 adb 可执行程序的路径
    if asst.connect('adb.exe', '127.0.0.1:5555'):
        print('连接成功')
    else:
        print('连接失败')
        exit()

    # 任务及参数请参考 docs/zh-cn/protocol/integration.md

    asst.append_task('StartUp')
    asst.append_task('Fight', {
        'stage': '',
        'report_to_penguin': True,
        # 'penguin_id': '1234567'
    })
    asst.append_task('Recruit', {
        'select': [4],
        'confirm': [3, 4],
        'times': 4
    })
    asst.append_task('Infrast', {
        'facility': [
            "Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm"
        ],
        'drones': "Money"
    })
    asst.append_task('Visit')
    asst.append_task('Mall', {
        'shopping': True,
        'buy_first': ['招聘许可', '龙门币'],
        'blacklist': ['家具', '碳'],
    })
    asst.append_task('Award')
    # asst.append_task('Copilot', {
    #     'filename': './GA-EX8-raid.json',
    #     'formation': False
    # })
    # asst.append_task('Custom', {"task_names": ["AwardBegin"]})
    asst.start()

    while asst.running():
        time.sleep(0)
