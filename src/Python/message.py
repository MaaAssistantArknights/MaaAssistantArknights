from enum import Enum, unique, auto



@unique
class Message(Enum):
    """
    回调消息类型
    
    注释部分仅展示常用消息的回调JSON格式，完全的回调JSON请参考C++侧实现代码
    """

    PtrIsNull = 0
    """
    指针为空
    """

    ImageIsEmpty = auto()
    """
    图像为空
    """

    WindowMinimized = auto()
    """
    [已弃用] 窗口被最小化了
    """

    InitFaild = auto()
    """
    初始化失败
    """

    TaskError = auto()
    """
    任务错误（任务一直出错，retry次数达到上限）
    """

    OcrResultError = auto()
    """
    Ocr识别结果错误
    """

    TaskStart = 1000
    """
    单个原子任务开始
    """

    TaskMatched = auto()
    """
    任务匹配成功
    """

    ReachedLimit = auto()
    """
    单个原子任务达到次数上限
    """

    ReadyToSleep = auto()
    """
    准备开始睡眠
    """

    EndOfSleep = auto()
    """
    睡眠结束
    """

    AppendProcessTask = auto()
    """
    [已弃用] 新增流程任务，Assistance内部消息，外部不需要处理
    """

    AppendTask = auto()
    """
    [已弃用] 新增任务，Assistance内部消息，外部不需要处理
    """

    TaskCompleted = auto()
    """
    单个原子任务完成

    :Json格式:
        { "name": string, "type": int, "exec_time": int, "max_times": int, "task_type" : string,  "algorithm" : int}
    
    :字段解释:
        ``name``        任务名
            :常见值:
            ``StartButton2``    开始行动（橙色按钮）
            ``MedicineConfirm`` 使用理智药
            ``StoneConfirm``    使用源石
            ``RecruitRefresh``  公招刷新标签

        ``type``        操作类型

        ``exec_times``  已执行次数

        ``max_times``   最大执行次数

        ``task_type``   任务类型

        ``algorithm``   算法类型
    """

    PrintWindow = auto()
    """
    截图消息
    """

    ProcessTaskStopAction = auto()
    """
    流程任务执行到了Stop的动作
    """

    TaskChainCompleted = auto()
    """
    任务链完成

    :Json格式:
        { "task_chain": string }

    :字段解释:
        ``task_chain``  任务链名称
    """

    ProcessTaskNotMatched = auto()
    """
    流程任务识别错误
    """

    AllTasksCompleted = auto()
    """
    所有任务完成

    :Json格式:
        null
    """

    TaskChainStart = auto()
    """
    开始任务链

    :Json格式:
        { "task_chain": string }
    :字段解释:
        ``task_chain``  任务链名称
            :常见值:    
                "Fight"     刷理智
                "Award"     领取每日奖励
                "Visit"     访问好友
                "Mall"      信用商店
                "Infrast"   基建
                "Recruit"   自动公招
                "RecruitCalc"   公招计算
    """

    TextDetected = 2000
    """
    识别到文字
    """

    ImageFindResult = auto()
    """
    查找图像的结果
    """

    ImageMatched = auto()
    """
    图像匹配成功
    """

    StageDrops = auto()
    """
    关卡掉落信息
    
    :Json格式:
    {"exceptions":[],"resultLabel":true,"drops":[{"dropType":"NORMAL_DROP","itemId":"2002","quantity":1,"itemName":"初级作战记录"},{"dropType":"NORMAL_DROP","itemId":"2003","quantity":1,"itemName":"中级作战记录"},{"dropType":"NORMAL_DROP","itemId":"2004","quantity":3,"itemName":"高级作战记录"}],"dropTypes":[{"dropTypes":"LMB"},{"dropTypes":"NORMAL_DROP"}],"stage":{"stageCode":"LS-5","stageId":"wk_kc_5"},"stars":3,"fingerprint":"598b253d3b24247b756f39202052375138233b75653c2a575a111f59462a2d413c092e50421a1731ff0c07b07809070e131715bfad4a44300a1c0e0a5d0b620c","md5":"43084d95fcf1c0a0cce3fdeab8c41334","cost":33.7536,"statistics":[{"itemId":"2004","itemName":"高级作战记录","count":15},{"itemId":"2002","itemName":"初级作战记录","count":5},{"itemId":"2003","itemName":"中级作战记录","count":5}]}
    
    :字段解释:
        ``drops``       当次掉落
        ``statistics``  掉落统计
    """

    RecruitTagsDetected = 3000
    """
    公招识别到了Tags

    :Json格式:
        {"tags":["近卫干员","狙击干员","医疗干员","新手","输出"]}
    """

    RecruitSpecialTag = auto()
    """
    公招识别到了特殊的Tag
    
    :Json格式:
        { "tag": string}

    字段解释
        ``tag`` 特殊Tag
            :常见值:
            "高级资深干员"
            "支援机械"
    """

    RecruitResult = auto()
    """
    公开招募结果

    :Json格式:
        {"result":[{"tags":["近卫干员","群攻"],"tag_level":3,"opers":[{"name":"幽灵鲨","level":5},{"name":"布洛卡","level":5},{"name":"艾丝黛尔","level":4},{"name":"泡普卡","level":3}]},{"tags":["群攻"],"tag_level":3,"opers":[{"name":"陨星","level":5},{"name":"送葬人","level":5},{"name":"幽灵鲨","level":5},{"name":"布洛卡","level":5},{"name":"远山","level":4},{"name":"格雷伊","level":4},{"name":"白雪","level":4},{"name":"艾丝黛尔","level":4},{"name":"泡普卡","level":3},{"name":"炎熔","level":3},{"name":"空爆","level":3}]},{"tags":["费用回复"],"tag_level":3,"opers":[{"name":"德克萨斯","level":5},{"name":"凛冬","level":5},{"name":"苇草","level":5},{"name":"清道夫","level":4},{"name":"红豆","level":4},{"name":"桃金娘","level":4},{"name":"翎羽","level":3},{"name":"香草","level":3},{"name":"芬","level":3}]},{"tags":["近卫干员"],"tag_level":3,"opers":[{"name":"诗怀雅","level":5},{"name":"星极","level":5},{"name":"因陀罗","level":5},{"name":"幽灵鲨","level":5},{"name":"布洛卡","level":5},{"name":"猎蜂","level":4},{"name":"杜宾","level":4},{"name":"艾丝黛尔","level":4},{"name":"慕斯","level":4},{"name":"霜叶","level":4},{"name":"缠丸","level":4},{"name":"泡普卡","level":3},{"name":"玫兰莎","level":3},{"name":"月见夜","level":3},{"name":"Castle-3","level":1}]},{"tags":["术师干员"],"tag_level":3,"opers":[{"name":"夜魔","level":5},{"name":"远山","level":4},{"name":"格雷伊","level":4},{"name":"夜烟","level":4},{"name":"史都华德","level":3},{"name":"炎熔","level":3},{"name":"12F","level":2},{"name":"杜林","level":2}]},{"tags":["术师干员","群攻"],"tag_level":3,"opers":[{"name":"远山","level":4},{"name":"格雷伊","level":4},{"name":"炎熔","level":3}]},{"tags":["新手"],"tag_level":2,"opers":[{"name":"黑角","level":2},{"name":"巡林者","level":2},{"name":"12F","level":2},{"name":"杜林","level":2},{"name":"夜刀","level":2}]},{"tags":["新手","术师干员"],"tag_level":2,"opers":[{"name":"12F","level":2},{"name":"杜林","level":2}]}],"maybe_level":3}

    :字段解释:
        ``result``      识别结果
        ``maybe_level`` 可能出的等级
    """

    RecruitSelected = auto()
    """
    选择了Tags

    :Json格式:
        {"tags":["术师干员","减速"]}
    """

    RecruitError = auto()
    """
    公招识别错误。
    """

    InfrastSkillsDetected = 4000
    """
    识别到了基建技能（当前页面）
    """

    InfrastSkillsResult = auto()
    """
    识别到的所有可用技能
    """

    InfrastComb = auto()
    """
    当前房间的最优干员组合
    """

    EnterFacility = auto()
    """
    进入某个房间
    
    :Json格式:
        { "facility": string, "index": int }
    :字段解释:
        ``facility``    房间名
        ``index``       第几个该类房间
    """

    # 
    FacilityInfo = auto()
    """
    当前设施信息
    """
