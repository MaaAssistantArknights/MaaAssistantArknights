from enum import Enum, unique, auto


@unique
class Msg(Enum):
    # Error Msg
    PtrIsNull = auto()              # 指针为空
    ImageIsEmpty = auto()           # 图像为空
    WindowMinimized = auto()        # [已弃用] 窗口被最小化了
    InitFaild = auto()              # 初始化失败
    TaskError = auto()              # 任务错误（任务一直出错，retry次数达到上限）
    OcrResultError = auto()         # Ocr识别结果错误
    # Info Msg: about Task
    TaskStart = 1000                # 任务开始
    TaskMatched = auto()            # 任务匹配成功
    ReachedLimit = auto()           # 单个原子任务达到次数上限
    ReadyToSleep = auto()           # 准备开始睡眠
    EndOfSleep = auto()             # 睡眠结束
    AppendProcessTask = auto()      # [已弃用] 新增流程任务，Assistance内部消息，外部不需要处理
    AppendTask = auto()             # [已弃用] 新增任务，Assistance内部消息，外部不需要处理
    TaskCompleted = auto()          # 单个原子任务完成
    PrintWindow = auto()            # 截图消息
    ProcessTaskStopAction = auto()  # 流程任务执行到了Stop的动作
    TaskChainCompleted = auto()     # 任务链完成
    ProcessTaskNotMatched = auto()  # 流程任务识别错误
    AllTasksCompleted = auto()      # 所有任务完成
    TaskChainStart = auto()         # 开始任务链
    # Info Msg: about Identify */
    TextDetected = 2000             # 识别到文字
    ImageFindResult = auto()        # 查找图像的结果
    ImageMatched = auto()           # 图像匹配成功
    StageDrops = auto()             # 关卡掉落信息
    # Open Recruit Msg */
    RecruitTagsDetected = 3000      # 公招识别到了Tags
    RecruitSpecialTag = auto()      # 公招识别到了特殊的Tag
    RecruitResult = auto()          # 公开招募结果
    RecruitSelected = auto()        # 选择了Tags
    # Infrast Msg */
    InfrastSkillsDetected = 4000    # 识别到了基建技能（当前页面）
    InfrastSkillsResult = auto()    # 识别到的所有可用技能
    InfrastComb = auto()            # 当前房间的最优干员组合
    EnterFacility = auto()          # 进入某个房间
    FacilityInfo = auto()           # 当前设施信息
