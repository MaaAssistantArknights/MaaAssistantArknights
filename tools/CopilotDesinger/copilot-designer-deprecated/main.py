from CopilotDesinger import *

# 使用样例 渊默行动 18
# 按道理是能用的，我照着作者的作业抄的，不能用一定是玛丽的问题（

# 阅读过此文会对使用本程序有很大的帮助（毕竟本来就是为了生成这种json文件的（
# https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E6%88%98%E6%96%97%E6%B5%81%E7%A8%8B%E5%8D%8F%E8%AE%AE.md

# （其实我觉得有个 ide 会点英文 应该是个人都会用，不过我还是稍微写点注释啥的

# 创建作业对象 要啥填啥就完事了 别问为什么用中文命名变量 我怕改成英文了自己过会都看不懂了（
危机合约十八 = Battle(stage_name="obt/rune/level_rune_11-01",
                version="v4.0",
                doc=Document("《全自动危机合约 18》", "作业抄自 https://www.bilibili.com/video/BV13S4y187NW"))
# 一键开启海嗣语（
危机合约十八.enable_haisi_doc()
# 创建要使用的干员（老婆们）
# 第一个参数是名字，第二个是几技能
史尔特尔 = 危机合约十八.create_operator("史尔特尔", 3)
伊芙利特 = 危机合约十八.create_operator("伊芙利特", 2)
# 鸡翅技能设定为好了就开
棘刺 = 危机合约十八.create_operator("棘刺", 3, SkillUsageType.UseWhenOk)
# 创建一个群组
对空高速单体狙 = 危机合约十八.create_group("对空高速单体狙", Operator("克洛丝", 1), Operator("能天使", 3))
红 = 危机合约十八.create_operator("红", 2)
蜜莓 = 危机合约十八.create_operator("蜜莓", 1)

# 开始写作业 QWQ
# 调成二倍速
危机合约十八.speed_up()

# 部署鸡翅 部署在(8,6)格子上，方向朝上
# 不知道哪格是哪格的可以先随便写点然后 save_as_json() 和 test()
# 运行后会在同目录下生成 map.png 的
棘刺.deploy((8, 6), Direction.UP)
对空高速单体狙.deploy((2, 4), Direction.UP)
伊芙利特.deploy((10, 4), Direction.LEFT)
蜜莓.deploy((10, 5), Direction.LEFT)
# 等待 16 杀后撤退
对空高速单体狙.wait_kills(16).retreat()
红.wait_kills(18).deploy((3, 5), Direction.LEFT)
史尔特尔.wait_kills(21).pre_delay(1500).deploy((2, 3), Direction.DOWN)
# 使用 42 技能
史尔特尔.use_skill()
史尔特尔.wait_kills(23).retreat()
红.wait_kills(25).pre_delay(10000).deploy((3, 5), Direction.LEFT)
红.retreat()
史尔特尔.wait_kills(29).rear_delay(1500).deploy((9, 4), Direction.LEFT)
红.pre_delay(1000).deploy((4, 4), Direction.DOWN)
史尔特尔.use_skill()
史尔特尔.wait_kills(34).retreat()
红.pre_delay(5000).deploy((3, 5), Direction.LEFT)

# print(json.dumps(危机合约十八.to_dict()))
# 保存为json文件
危机合约十八.save_as_json(stage_name="危机合约18")
# 直接进行测试 要用的需要点进去配置一下 不然直接注释掉也行（
# 危机合约十八.test()
