import ctypes
import pathlib
import platform

from message import Message


class Asst:
    CallBackType = ctypes.CFUNCTYPE(
        None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)
    """
    回调函数，使用实例可参照my_callback

    :params:
        ``param1 message``: 消息类型
        ``param2 details``: gbk json string
        ``param3 arg``:     自定义参数
    """

    def __init__(self, dirname: str, callback: CallBackType, arg=None):
        """
        :params:
            ``dirname``:    DLL及资源所在文件夹路径
            ``callback``:   回调函数
            ``arg``:        自定义参数
        """
        if platform.system().lower() == 'windows':
            self.__libpath = pathlib.Path(dirname) / 'MeoAssistant.dll'
            self.__lib = ctypes.WinDLL(str(self.__libpath))
        else:
            self.__libpath = pathlib.Path(dirname) / 'libMeoAssistant.so'
            self.__lib = ctypes.CDLL(str(self.__libpath))
        self.__set_lib_properties()

        self.__ptr = self.__lib.AsstCreateEx(
            dirname.encode('utf-8'), callback, arg)

    def __del__(self):
        self.__lib.AsstDestroy(self.__ptr)

    def connect(self, adb_path: str, address: str, config: str = 'General'):
        """
        连接设备

        :params:
            ``adb_path``:   adb 程序的路径
            ``address``:    adb 地址+端口
            ``config``:     adb 配置，可参考 resource/config.json

        :return: 是否连接成功
        """
        return self.__lib.AsstConnect(self.__ptr,
                                      adb_path.encode('utf-8'), address.encode('utf-8'), config.encode('utf-8'))

    def append_fight(self, stage: str, max_medicine: int, max_stone: int, max_times: int) -> bool:
        """
        添加刷理智任务

        :params:
            ``stage``:              关卡名，为空则不进行选关（需要当前在蓝色开始按钮界面），否则跳转到对应关卡
                                        支持：''、'LastBattle'、'Annihilation'、'CE-5'、'AP-5'、'LS-5'、'CA-5'
            ``max_medicine``:       最多吃多少理智药
            ``max_stone``:          最多吃多少源石
            ``max_times``:          最多刷多少
        """
        return self.__lib.AsstAppendFight(self.__ptr, stage.encode('utf-8'), max_medicine, max_stone, max_times)

    def append_award(self) -> bool:
        """
        添加领取每日奖励任务
        """
        return self.__lib.AsstAppendAward(self.__ptr)

    def append_visit(self) -> bool:
        """
        添加访问好友任务
        """
        return self.__lib.AsstAppendVisit(self.__ptr)

    def append_mall(self, with_shopping: bool) -> bool:
        """
        添加信用商店任务

        :params:
            ``with_shopping``:    是否信用商店购物
        """
        return self.__lib.AsstAppendMall(self.__ptr, with_shopping)

    def append_infrast(self, work_mode: int, order_list: list, uses_of_drones: str, dorm_threshold: float) -> bool:
        """
        添加基建任务

        :params:
            ``work_mode``:          工作模式，当前仅支持传 1
            ``order_list``:         换班顺序，str list，例如 [ "Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm"]
            ``uses_of_drones``:     无人机用途，支持以下之一："_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
            ``dorm_threshold``:     宿舍心情阈值，取值 [0, 1.0]
        """
        order_byte_list = []

        for item in order_list:
            order_byte_list.append(item.encode('utf-8'))

        order_len = len(order_byte_list)
        order_arr = (ctypes.c_char_p * order_len)()
        order_arr[:] = order_byte_list

        return self.__lib.AsstAppendInfrast(self.__ptr, work_mode,
                                            order_arr, order_len,
                                            uses_of_drones.encode('utf-8'), dorm_threshold)

    def append_recruit(self, max_times: int, select_level: list, confirm_level: list, need_refresh: bool, use_expedited: bool) -> bool:
        """
        添加自动公招任务

        :params:
            ``max_times``:       自动公招次数
            ``select_level``:    要点击的Tags等级（例如3级Tags不选也可以直接确认），int list
            ``confirm_level``:   要确认开始招募的Tags等级，int list
            ``need_refresh``:    是否刷新3级Tags
            ``use_expedited``:   是否使用加急许可
        """
        select_len = len(select_level)
        select_arr = (ctypes.c_int * select_len)()
        select_arr[:] = select_level

        confirm_len = len(confirm_level)
        confirm_arr = (ctypes.c_int * confirm_len)()
        confirm_arr[:] = confirm_level

        return self.__lib.AsstAppendRecruit(self.__ptr, max_times,
                                            select_arr, select_len,
                                            confirm_arr, confirm_len,
                                            need_refresh, use_expedited)

    def append_roguelike(self, mode: int) -> bool:
        """
        添加无限肉鸽任务

        :params:
            ``mode``:   工作模式 : 
                    0 - 尽可能一直往后打
                    1 - 第一层投资完源石锭就退出
                    2 - 投资过后再退出，没有投资就继续往后打
        """

        return self.__lib.AsstAppendRoguelike(self.__ptr, mode)

    def start(self) -> bool:
        """
        开始任务
        """
        return self.__lib.AsstStart(self.__ptr)

    def start_recruit_calc(self, select_level: list, set_time: bool) -> bool:
        """
        进行公招计算

        :params:
            ``select_level``:    要点击的Tags等级，int list
            ``set_time``:        是否要点击9小时
        """
        select_len = len(select_level)
        select_arr = (ctypes.c_int * select_len)()
        select_arr[:] = select_level

        return self.__lib.AsstStartRecruitCalc(self.__ptr, select_arr, select_len, set_time)

    def stop(self) -> bool:
        """
        停止并清空所有任务
        """
        self.__lib.AsstStop.restype = ctypes.c_bool
        self.__lib.AsstStop.argtypes = (ctypes.c_void_p,)
        return self.__lib.AsstStop(self.__ptr)

    def set_penguin_id(self, id: str) -> bool:
        """
        设置企鹅物流ID

        :params:
            ``id``:     企鹅物流ID，仅数字部分
        """

        return self.__lib.AsstSetPenguinId(self.__ptr, id.encode('utf-8'))

    def log(self, level: str, message: str):
        '''
        打印日志

        :params:
            ``level``:      日志等级标签
            ``message``:    日志内容
        '''

        return self.__lib.AsstLog(self.__ptr, level.encode('utf-8'), message.encode('utf-8'))

    def get_version(self) -> str:
        """
        获取DLL版本号

        :return: 版本号
        """
        return self.__lib.AsstGetVersion().decode('utf-8')

    def __set_lib_properties(self):
        self.__lib.AsstCreateEx.restype = ctypes.c_void_p
        self.__lib.AsstCreateEx.argtypes = (
            ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p,)

        self.__lib.AsstDestroy.argtypes = (ctypes.c_void_p,)

        self.__lib.AsstConnect.restype = ctypes.c_bool
        self.__lib.AsstConnect.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,)

        self.__lib.AsstAppendFight.restype = ctypes.c_bool
        self.__lib.AsstAppendFight.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int,)

        self.__lib.AsstAppendAward.restype = ctypes.c_bool
        self.__lib.AsstAppendAward.argtypes = (ctypes.c_void_p,)

        self.__lib.AsstAppendVisit.restype = ctypes.c_bool
        self.__lib.AsstAppendVisit.argtypes = (ctypes.c_void_p,)

        self.__lib.AsstAppendMall.restype = ctypes.c_bool
        self.__lib.AsstAppendMall.argtypes = (ctypes.c_void_p, ctypes.c_bool)

        self.__lib.AsstAppendInfrast.restype = ctypes.c_bool
        self.__lib.AsstAppendInfrast.argtypes = (
            ctypes.c_void_p, ctypes.c_int,
            ctypes.POINTER(ctypes.c_char_p), ctypes.c_int,
            ctypes.c_char_p, ctypes.c_double,)

        self.__lib.AsstAppendRecruit.restype = ctypes.c_bool
        self.__lib.AsstAppendRecruit.argtypes = (
            ctypes.c_void_p, ctypes.c_int,
            ctypes.POINTER(ctypes.c_int), ctypes.c_int,
            ctypes.POINTER(ctypes.c_int), ctypes.c_int,
            ctypes.c_bool, ctypes.c_bool,)

        self.__lib.AsstAppendRoguelike.restype = ctypes.c_bool
        self.__lib.AsstAppendRoguelike.argtypes = (
            ctypes.c_void_p, ctypes.c_int,)

        self.__lib.AsstStart.restype = ctypes.c_bool
        self.__lib.AsstStart.argtypes = (ctypes.c_void_p,)

        self.__lib.AsstStartRecruitCalc.restype = ctypes.c_bool
        self.__lib.AsstStartRecruitCalc.argtypes = (
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_bool,)

        self.__lib.AsstSetPenguinId.restype = ctypes.c_bool
        self.__lib.AsstSetPenguinId.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p)

        self.__lib.AsstGetVersion.restype = ctypes.c_char_p

        self.__lib.AsstLog.restype = ctypes.c_none
        self.__lib.AsstLog.argtypes = (
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p)
