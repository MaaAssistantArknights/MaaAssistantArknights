import json
import pathlib
import platform

from interface import Asst, Message

if __name__ == "__main__":

    @Asst.CallBackType
    def my_callback(msg, details, arg):
        m = Message(msg)
        if platform.system().lower() == 'windows':
            d = json.loads(details.decode('gbk'))
        else:
            d = json.loads(details.decode('utf-8'))

        print(m, d, arg)

    if platform.system().lower() == 'windows':
        dirname: str = (pathlib.Path.cwd().parent.parent / 'x64' / 'Release').__str__()
    else:
        dirname: str = (pathlib.Path.cwd().parent.parent / 'build').__str__()

    asst = Asst(dirname=dirname, callback=my_callback)

    print('version', asst.get_version())

    if asst.catch_custom('127.0.0.1:7555'):
        print('连接成功')
    else:
        print('连接失败')
        exit()

    asst.append_fight("CE-5", 0, 0, 1)
    # asst.append_infrast(1, [ "Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm"], "Money", 0.3)
    # asst.append_award()
    asst.start()

    # asst.start_recruit_calc([4, 5, 6], True)

    while True:
        input('>')
