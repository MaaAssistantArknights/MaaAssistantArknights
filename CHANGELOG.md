## v5.24.0-beta.1

### 新增 | New

* 在启动时检测额外的DLL文件 (#13850) @Rbqwow @ABA2396
* 支持一键悖论模拟 (#13893) @status102 @Constrat @pre-commit-ci[bot] @momomochi987 @HX3N @Manicsteiner @ABA2396
* 争锋频道：蜜果城 @ABA2396
* 自动战斗关卡结束后增加识别使用黑色返回按钮的检测 @status102
* 更新源为海外源时隐藏 Mirror酱 CDK 输入框 @ABA2396

### 改进 | Improved

* 任务baseTask @status102
* 重构自动战斗战斗列表 (#13852) @status102 @pre-commit-ci[bot]
* 更新 CsWin32 @ABA2396
* ToastNotification 重构 @ABA2396
* 向战斗队列中添加SSS作业时追加错误输出 @status102
* limit codeql run only to source code (#13802) @Constrat

### 修复 | Fix

* var type @status102
* YostarKR EA navigation @HX3N
* 勾八导航乱写 @ABA2396
* 自动战斗界面Idle setter限制 @status102
* 自动战斗-战斗列表使用错误的作业战斗 @status102
* filters @ABA2396
* 信用购物有时候点不掉危机合约支援界面 @ABA2396
* ServerChan 未使用通用 httpService @ABA2396
* 错误清除 _tasksStatus 的时机 @ABA2396
* 肉鸽入口等待 ocr 识别错误 @ABA2396
* 界园肉鸽商店钱包余额ocr roi @status102
* 自动战斗-战斗列表`突袭`提示显示在运行期显示效果 @status102
* 不再点击停止任务时移除存储的TaskId @status102
* 繁中服薩米肉鴿第四層名稱 (#13846) @momomochi987
* 肉鸽投资存款不变重试20次退出 @status102
* required version 显示错误 @ABA2396
* 会客室换班失败（如选人过程中弹出线索交流完成）时重开任务 @ABA2396
* 肉鸽开局招募在无开局干员时, 无法识别两招募的左侧位 @status102
* 千古鸭帝进战斗 @Saratoga-Official
* 日服的萨卡兹第五层怎么全 jb 改成 exit 了 @ABA2396
* 自动战斗干员编队状态; 修复编队中干员属性未达标时反复报错; 缺少干员输出时, 干员组内干员打平输出 (#13795) @status102
* 修正生息演算中 "丰饶灌木林" 的 OCR 识别 (#13825) @Alan-Charred
* 资源更新文件处理 @hguandl
* 萨卡兹肉鸽因动画延时偶发无法领取去伪存真合成物品 @ABA2396
* 萨米投资进二层会卡在预见密文板 @ABA2396

### 其他 | Other

* YostarJP new main stage navigation and ocr fix (#13902) @Manicsteiner
* 悖论模拟使用多任务流程 @status102
* remove ParadoxMode var @status102
* 移除MaaCore不再支持的自动战斗作业代码解析 @status102
* 删掉 core 的联网功能 @ABA2396
* dll 检测排除带 maa 的 dll @ABA2396
* core 删除 cpr @ABA2396
* YostarJP EA navigation @Manicsteiner
* YostarEN EA navigation @Constrat
* YostarKR EA navigation (#13888) @HX3N
* 自动战斗模组错误本地化 & 移除多余的行末空格 (#13800) @soundofautumn @Constrat @HX3N
* 调整小游戏内容顺序 @ABA2396
* 修改 RA 和肉鸽的进入终端等待时间 @ABA2396
* 调整萨卡兹，界园的古米精二优先度 @Saratoga-Official
* 修改备份文件写入时间 @ABA2396
* 关机/休眠/睡眠 统一调用 PowerManagement @ABA2396
* 调整 GetModuleFileName 判断 @ABA2396
* luid @ABA2396
* 移除未使用的 container @ABA2396
* A single-line comment within C# code is not preceded by a blank line. @ABA2396
* A C# partial element is missing a documentation header. @ABA2396
* An item within a C# enumeration is missing an Xml documentation header. @ABA2396
* A C# method, constructor, delegate or indexer element is missing documentation for one or more of its parameters. @ABA2396
* The XML header documentation for a C# element is missing a tag. @ABA2396
* Copyright 2025 @ABA2396
* mumu 使用 7555 端口时禁用 Index 检测，添加日志警告 @ABA2396
* file header @status102
* 自动编队助战的确实干员读取 @status102
* 加点肉鸽事件 fallback 前延迟，可能减轻点网络波动影响 @ABA2396
* 自动战斗不再使用`need_navigate`字段作为导航启用开关, 而是直接使用`navigate_name`字段 @status102
* 调整技能截图判断 @ABA2396
