## v5.16.6

### 新增 | New

* 显示资源关卡全开放剩余时间 @ABA2396
* 更新解决方案 @ABA2396
* ProcessTask支持特征匹配 (#12701) @status102

### 改进 | Improved

* 自动战斗编队时增加干员组名输出 @status102
* 黄票绿票商店使用特征匹配 (#12718) @status102
* 尚蜀夜市招募黑名单添加新约能天使 (#12734) @Black1312

### 修复 | Fix

* 矢量突破开始行动 @ABA2396
* 自动战斗编队时重复选中同一干员 @status102
* 部分情况下刷理智连战使用AUTO模式时, 吃理智药溢出理智 @status102
* 开始行动 点击过快 @Daydreamer114
* 开始行动 ocr replace @Daydreamer114
* 指定战斗次数后剩余次数小于连战次数时，可能修改到非指定连战次数 @status102
* IS4 竣工仪式 @Daydreamer114
* 繁中服悖論模擬 (#12711) @vonnoq
* SSSBuffChoose for YostarEN @Constrat

### 其他 | Other

* FeatureMatch任务无法读取roi @status102
* remove cachie from optimize templates @Constrat
* update-resources.yml removal from res-update-game @Constrat
* 延迟停止按钮叠加显示 @ABA2396
* ocr fix (#12736) @Saratoga-Official
* ci format @ABA2396
* mac platform @status102
* 简化基建任务自定义配置路径Readonly @status102
* 重构干员识别 @ABA2396
* 修改 not in deploy plan 提示 @ABA2396
* point_states 改用 unordered_map @ABA2396
* 调整截图保存逻辑 @ABA2396
* 黄绿票商店任务整理 @status102
* YostarJP ocr fix (#12729) @Manicsteiner
* find OpenCV features2d and xfeatures2d components (#12714) @wangl-cc
* ota.maa.plus -> api.maa.plus @MistEO
* 调整 prts 颜色 @ABA2396
* 添加几个专精 ocrReplace @ABA2396
