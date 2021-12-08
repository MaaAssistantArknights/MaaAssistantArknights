import ctypes
import os
import json
import pathlib
from msg import Msg





class Asst:
    CallBackType = ctypes.CFUNCTYPE(
        None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)

    def __init__(self, dirname: str, callback: CallBackType, arg=None):
        os.chdir(dirname)
        self.__dllpath = pathlib.Path(dirname) / 'MeoAssistance.dll'
        self.__dll = ctypes.WinDLL(str(self.__dllpath))

        self.__dll.AsstCreateEx.restype = ctypes.c_void_p
        self.__dll.AsstCreateEx.argtypes = (ctypes.c_void_p, ctypes.c_void_p,)
        self.__ptr = self.__dll.AsstCreateEx(callback, arg)

    def __del__(self):
        self.__dll.AsstDestroy.argtypes = (ctypes.c_void_p,)
        self.__dll.AsstDestroy(self.__ptr)

    def catch_default(self) -> bool:
        self.__dll.AsstCatchDefault.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstCatchDefault(self.__ptr)

    def catch_emulator(self) -> bool:
        self.__dll.AsstCatchEmulator.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstCatchEmulator(self.__ptr)

    def catch_custom(self, address: str) -> bool:
        self.__dll.AsstCatchCustom.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p,)
        return self.__dll.AsstCatchCustom(self.__ptr, address)

    def append_fight(self, max_medicine: int, max_stone: int, max_times: int) -> bool:
        self.__dll.AsstAppendFight.argtypes = (
            ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int,)
        return self.__dll.AsstAppendFight(self.__ptr, max_medicine, max_stone, max_times)

    def append_award(self) -> bool:
        self.__dll.AsstAppendAward.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstAppendAward(self.__ptr)

    def append_visit(self) -> bool:
        self.__dll.AsstAppendVisit.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstAppendVisit(self.__ptr)

    def append_mall(self, with_shopping: bool) -> bool:
        self.__dll.AsstAppendMall.argtypes = (ctypes.c_void_p, ctypes.c_bool)
        return self.__dll.AsstAppendMall(self.__ptr, with_shopping)

    def append_infrast(self, work_mode: int, order_list: list, uses_of_drones: str, dorm_threshold: float) -> bool:
        order_len = len(order_list)
        order_arr = (ctypes.c_char_p * order_len)()
        order_arr[:] = order_list

        self.__dll.AsstAppendInfrast.argtypes = (
            ctypes.c_void_p, ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p), ctypes.c_int,
            ctypes.c_char_p, ctypes.c_double,)
        return self.__dll.AsstAppendInfrast(self.__ptr, work_mode,
                                            order_arr, order_len,
                                            uses_of_drones, dorm_threshold)

    def append_recruit(self, max_times: int, select_level: list, confirm_level: list, need_refresh: bool) -> bool:
        select_len = len(select_level)
        select_arr = (ctypes.c_int * select_len)()
        select_arr[:] = select_level

        confirm_len = len(confirm_level)
        confirm_arr = (ctypes.c_int * confirm_len)()
        confirm_arr[:] = confirm_level

        self.__dll.AsstAppendRecruit.argtypes = (
            ctypes.c_void_p, ctypes.c_int,
            ctypes.POINTER(ctypes.c_int), ctypes.c_int,
            ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_bool,)
        return self.__dll.AsstAppendRecruit(self.__ptr, max_times,
                                            select_arr, select_len,
                                            confirm_arr, confirm_len, need_refresh)

    def start(self) -> bool:
        self.__dll.AsstStart.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstStart(self.__ptr)

    def start_recurit_clac(self, select_level: list, set_time: bool) -> bool:
        select_len = len(select_level)
        select_arr = (ctypes.c_int * select_len)()
        select_arr[:] = select_level

        self.__dll.AsstStartRecruitCalc.argtypes = (
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_bool,)
        return self.__dll.AsstStartRecruitCalc(self.__ptr, select_arr, select_len, set_time)

    def stop(self) -> bool:
        self.__dll.AsstStop.argtypes = (ctypes.c_void_p,)
        return self.__dll.AsstStop(self.__ptr)

    def set_penguin_id(self, id: str) -> bool:
        self.__dll.AsstSetPenguinId.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p)
        return self.__dll.AsstSetPenguinId(self.__ptr, id)

    def get_version(self) -> str:
        self.__dll.AsstGetVersion.restype = ctypes.c_char_p
        return self.__dll.AsstGetVersion()

    def main_loop(self) -> None:
        print(self.__dict__)
        from time import sleep
        while True:
            sleep(1)

@Asst.CallBackType
def my_callback(msg, details, arg):
    d = json.loads(details.decode('gbk'))
    print(Msg(msg), d, arg)


if __name__ == "__main__":

    dirname: str = (pathlib.Path.cwd().parent.parent / 'x64' / 'Release').__str__()
    asst = Asst(dirname=dirname, callback=my_callback)
    if asst.catch_default():
        print('连接成功')
    else:
        print('连接失败')
        os.system.exit()

    # asst.append_fight(0, 0, 1)
    # asst.start()

    asst.start_recurit_clac([4, 5, 6], True)

    asst.main_loop()
