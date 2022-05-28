from CopilotDesinger import Battle, Document, Operator, SkillUsageType, Direction

危机合约十八 = Battle("obt/rune/level_rune_11-01",
                doc=Document("《全自动危机合约 18》", "作业抄自 https://www.bilibili.com/video/BV13S4y187NW"))

史尔特尔 = 危机合约十八.create_operator("史尔特尔", 3)
伊芙利特 = 危机合约十八.create_operator("伊芙利特", 2)
棘刺 = 危机合约十八.create_operator("棘刺", 3, SkillUsageType.UseWhenOk)
对空高速单体狙 = 危机合约十八.create_group("对空高速单体狙", Operator("克洛丝", None, 1), Operator("能天使", None, 3))
红 = 危机合约十八.create_operator("红", 2)
蜜莓 = 危机合约十八.create_operator("蜜莓", 1)

危机合约十八.speed_up()
棘刺.deploy((8, 6), Direction.UP)
对空高速单体狙.deploy((2, 4), Direction.UP)
伊芙利特.deploy((10, 4), Direction.LEFT)
蜜莓.deploy((10, 5), Direction.LEFT)
对空高速单体狙.wait_kills(16).retreat()
红.wait_kills(18).deploy((3, 5), Direction.LEFT)
史尔特尔.wait_kills(21).rear_delay(1500).deploy((2, 3), Direction.DOWN)
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
危机合约十八.save_as_json("YanFengRongDong.json")
