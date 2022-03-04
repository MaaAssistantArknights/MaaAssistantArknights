"""
function: 定时启动 maa
author: Black Cat Bon
version: v2.0.1
"""

from importlib.resources import path
import threading
import time
import schedule

import datetime
import json
import pathlib
import sys
import re

from interface import Asst, Message


def get_now_time():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def job():
    # print("I'm running on thread %s" % threading.current_thread())
    print("现在时间：" + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    asst.append_fight("LastBattle", 0, 0, 999)
    asst.append_recruit(3, [4, 5], [3, 4, 5], True, False)
    asst.append_infrast(1, ["Mfg", "Trade", "Control",
                        "Power", "Reception", "Office", "Dorm"], "Money", 0.1)
    asst.append_visit()
    asst.append_mall(True)
    asst.append_award()
    # asst.set_penguin_id('1234567')

    asst.start()


def run_threaded(job_func):
    job_thread = threading.Thread(target=job_func)
    job_thread.start()


def get_target_time(input_time):
    matchObj = re.match('\d\d:\d\d', input_time)
    if matchObj:
        target_time = input_time
    else:
        while True:
            input_str = input(
                'Invalid time format, please input again!\n For example, 20:00 \n> ')
            if (input_str == 'exit()'):
                sys.exit()

            matchObj = re.match('\d\d:\d\d', input_str)

            if matchObj:
                break
            else:
                print('Please input a valid time!')

        target_time = input_str

    return target_time


def get_target_port(input_port):
    matchObj = re.match('\d\d\d\d', input_port)
    if matchObj:
        target_time = input_port
    else:
        while True:
            input_str = input(
                'Invalid port format, please input again!\n For example, 5565 \n> ')
            if (input_str == 'exit()'):
                sys.exit()

            matchObj = re.match('\d\d\d\d', input_str)

            if matchObj:
                break
            else:
                print('Please input a valid time!')

        target_time = input_str

    return target_time


if __name__ == "__main__":
    global target_time
    global asst

    if (len(sys.argv) == 1):
        target_time = '20:00'
        port = '5555'
    elif len(sys.argv) == 2:
        target_time = get_target_time(sys.argv[1])
        port = '5555'
    elif len(sys.argv) == 3:
        target_time = get_target_time(sys.argv[1])
        port = get_target_port(sys.argv[2])

    # asst 的回调
    @Asst.CallBackType
    def my_callback(msg, details, arg):
        m = Message(msg)
        d = json.loads(details.decode('utf-8'))

        print(m, d, arg)

    path: str = (pathlib.Path.cwd()).__str__()
    Asst.load(path)

    asst = Asst(callback=my_callback)

    print('version', asst.get_version())

    if asst.catch_custom('127.0.0.1:' + port):
        print('连接成功')
    else:
        print('连接失败')
        exit()

    schedule.every().day.at(target_time).do(run_threaded, job)
    # schedule.every(30).seconds.do(run_threaded, job)
    print("定时任务已设置，将于每天 " + target_time + " 运行")

    # print('下次定时任务开始于：' + target_time)

    while True:
        schedule.run_pending()
        time.sleep(1)
