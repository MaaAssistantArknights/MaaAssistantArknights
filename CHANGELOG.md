## v5.23.0-beta.1

### 新增 | New

* 烧水模式启用快速退出 (#13690) @SherkeyXD @pre-commit-ci[bot] @ABA2396
* 支持 梦乡 主题 (#13693) @ABA2396 @pre-commit-ci[bot]
* ADB 连接失败时尝试启动模拟器 勾选框在连接设置与启动设置中同时显示 @ABA2396
* 支持 mumu 5555 端口使用控制台退出与截图增强 @ABA2396
* 自动编队支持指定使用编队 @ABA2396

### 改进 | Improved

* 幸运掉落用家具零件代替显示 @ABA2396
* 仓库识别改用二值化后的非char模型识别 (#13667) @ABA2396
* 繁中服基建 + 保全派駐 7 相關更新 (#13643) @momomochi987 @pre-commit-ci[bot]
* 代理倍率、肉鸽难度 使用 `不切换` 代替 `当前`、`不使用` 以符合实际逻辑 (#13612) @status102 @HX3N @Constrat

### 修复 | Fix

* 修复萨卡兹肉鸽部分字段无法正确识别的问题 (#13691) @THSLP13 @pre-commit-ci[bot]
* 无法区分界园 3/5 层 boss @ABA2396
* 因招募动画卡预见密文板 (#13680) @ABA2396 @pre-commit-ci[bot]
* 获得排异反应的干员无法选择技能 @ABA2396
* 未启用备选关卡也会输出剿灭提醒 @status102
* EN Phantom IS font change nextlevel fix @Constrat
* 合成密文板未进入合成页面导致卡死 @ABA2396
* start plugin task not enough delay (#13623) @Constrat
* 商店因为动画卡在源石锭不足 @ABA2396
* 理智药背景干扰使用数量识别 @status102
* 第一层没出商店时会在第三层才退出 @ABA2396
* 肉鸽种子选择框visibility判断更新 @status102
* 每次截图测试前断开模拟器连接以获取最新的最快截图耗时 @ABA2396
* 锻冶旧迹 导航识别错误 @ABA2396
* 追加自定干员职业与最后编入干员职业相同时, 误展开子职业列表 @status102
* 肉鸽烧水选择当前难度时禁用 0难烧水切换 @status102

### 文档 | Docs

* mumu (#13658) @Rbqwow
* 文档站小修复 (#13659) @Rbqwow

### 其他 | Other

* 肉鸽任务高级设置界面xaml错误修复 @status102
* 修复fody一直报warning @SherkeyXD
* add slight delay before `InfrastEnteredFlag' in case of stutters in base rendering @Constrat
* 修修ocr bin threshold 被task info 错误覆盖 @status102
* schema error @ABA2396
* 移除使用empty.png当JustReturn屎 (#13652) @status102
* 识别错误等待+200ms @status102
* 肉鸽投资可能由于存款被长期遮挡导致提前退出 @status102
* 优化标题栏显示效果 @ABA2396
* 版本后面也跟个日期 @ABA2396
* 反转网络桥接提示框的按钮位置，避免误勾选 @ABA2396
* 调整自动战斗页面布局 @ABA2396
* 调整编队界面翻译 @ABA2396
* 合并regex @status102
* 更新nuke版本 @SherkeyXD
* 更新nuke构建参数信息 @SherkeyXD
* 更新MaaBuilder中使用的过时方法 @SherkeyXD
* wpf肉鸽难度初始化优化 @status102
* 迁移UpdateStageList @status102
* _stageManager迁移 @status102
* binThreshold, useRaw直接写入task @status102
* 繁中服「出蒼白海」導航入口文字調整 (#13681) @momomochi987
* 调整 LDExtras 文件位置 @ABA2396
* 自动战斗存图阈值改为0.75 @ABA2396
* 刷理智任务指定材料不再使用双属性保存，仅保存物品id @status102
* 密文板加一个进入后检查 @ABA2396
* 繁中服「出蒼白海」活動導航 (#13644) @momomochi987
* YostarKR CharsNameOcrReplace ocr edit @HX3N
* 添加null检测 @SherkeyXD
* rename MinimumFeatureLevel to minimumFeatureLevel @SherkeyXD
* add copyright header @SherkeyXD
* add nullable flags @SherkeyXD
* 合并ocrBinThreshold (#13635) @status102
* gitignore添加MaaBuilder相关内容 @SherkeyXD
* add missing png suffix to templates @SherkeyXD
* TestLinkAndGetImage 不允许同时运行 @ABA2396
* 模板轮换 (#13625) @status102
* 基建设施排序初始化 @status102
* wpf任务名字段统一 @status102
* 基建设施种类 @status102
* CustomTask存储 @status102
* 浅调一下初始化顺序 @status102
