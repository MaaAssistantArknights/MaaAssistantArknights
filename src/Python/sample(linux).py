def tasks():

    #infrast(mode=10000, plan_index=0, filename="")
    
    #visit()
   
    #mall()
    
    #recruit(select=[4, 5], confirm=[3, 4, 5])
    
    #fight(stage="", medicine=0) # stage="1-7", if null then return previous stage.
    
    roguelike(theme="Mizuki", squad="", roles="", core_char="")
    
    #award()
    
    #copilot() # 自动战斗

# adb 路径和 avd id 设置：一般不需要更改，若出现 “连接失败” 再进行更改

from os import getuid
from pwd import getpwuid
adb_path = "/home/" + getpwuid(getuid())[0] + "/Android/Sdk/platform-tools/adb"
avd_id = "emulator-5554"

# 具体任务设置，具体见集成文档

def infrast(mode=0, plan_index=0, filename=""):
    asst.append_task("Infrast", {
        "mode": mode,
        "facility": [
            "Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm"
        ],
        "drones": "Money",
        "threshold": 0.8,
        "filename": filename,
        "plan_index": plan_index
    })
def visit():
    asst.append_task("Visit")
def mall():
    asst.append_task("Mall", {
        "shopping": True,
        "buy_first": ["赤金", "招聘许可", "龙门币", "固源岩"],
        "blacklist": ["家具", "碳", "碳素", "碳素组", "加急许可"],
    })
def recruit(select=[4, 5], confirm=[4, 5]):
    asst.append_task("Recruit", {
        "refresh": True,
        "select": select,
        "confirm": confirm,
        "times": 4
    })
def fight(stage="", medicine=0):
    asst.append_task("Fight", {
        "stage": stage,
        "medicine": medicine,
        "report_to_penguin": True,
        # "penguin_id": "1234567"
    })
def roguelike(theme="Mizuki", squad="", roles="", core_char=""):
    asst.append_task("Roguelike", {
        "theme": theme, # Phantom
        "squad": squad,
        "roles": roles,
        "core_char": core_char,
    })
def award():
    asst.append_task("Award")
def copilot():
    asst.append_task("Copilot", {
        "filename": "./GA-EX8-raid.json",
        "formation": False
    })

# Updater is unstable, if you want to use it turn to true

use_updater = False


# 无需修改以下内容 ################################

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
    d = json.loads(details.decode("utf-8"))

    print(m, d, arg)

if __name__ == "__main__":

    # 请设置为存放 dll 文件及资源的路径
    path = pathlib.Path(__file__).parent.parent

    if use_updater:
        # 设置更新器的路径和目标版本并更新
        Updater(path, Version.Stable).update()

    # 外服需要再额外传入增量资源路径，例如
    # incremental_path=path / "resource" / "global" / "YoStarEN"
    Asst.load(path=path)

    # 若需要获取详细执行信息，请传入 callback 参数
    # 例如 asst = Asst(callback=my_callback)
    asst = Asst()

    # 设置额外配置
    # 触控方案配置
    asst.set_instance_option(InstanceOptionType.touch_type, "maatouch")
    # 暂停下干员
    # asst.set_instance_option(InstanceOptionType.deployment_with_pause, "1")

    # 启动模拟器。例如启动蓝叠模拟器的多开Pie64_1，并等待30s
    # Bluestacks.launch_emulator_win(r"C:\Program Files\BlueStacks_nxt\HD-Player.exe", 30, "Pie64_1")

    # 获取Hyper-v蓝叠的adb port
    # port = Bluestacks.get_hyperv_port(r"C:\ProgramData\BlueStacks_nxt\bluestacks.conf", "Pie64_1")

    # 请自行配置 adb 环境变量，或修改为 adb 可执行程序的路径

    if asst.connect(adb_path, avd_id):
        print("连接成功")
    else:
        print("连接失败")
        exit()

    # 任务及参数请参考 docs/集成文档.md

    asst.append_task("StartUp")

    tasks()

    #asst.append_task("Custom", {"task_names": ["AwardBegin"]})
    
    asst.start()

    while asst.running():
        time.sleep(0)
