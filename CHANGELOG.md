## v4.26.0-beta.2

### 新增

- 声明 baseTask 的任务直接覆盖同名任务 (#7016) @zzyyyl
- IS 关卡复刻 @ABA2396
- 繁中服 保全派駐 (#7021) @vonnoq
- 繁中服「照我以火」活動導航 (#7002) @momomochi987 @Constrat @zzyyyl
- 日服保全派驻 (#7012) @Manicsteiner
- 开始任务后强制锁定当前基建配置，强制定时启动时强制刷新基建配置 @ABA2396
- 萨米肉鸽使用密文板 (#6949) @Lancarus @MistEO
- 增加任务截图插件 (#6973) @zzyyyl

### 改进

- perf 优化萨米肉鸽部分关卡策略 @Lancarus
- 优化水月肉鸽策略 (#7043) @Yumi0606
- perf & typo: removed redundant variable & variable typo (#7032) @Constrat
- perf & refactor: 重构 tools/AutoLocalization (#6995) @quyansiyuanwang @Constrat
- 优化部分肉鸽逻辑 (#7004) @Lancarus
- 当清空自动战斗-战斗列表中的所有任务时，移除缓存中的战斗列表文件夹 (#6994) @status102
- TaskData 优化 (#6844) @zzyyyl

### 修复

- fix some warnings from visual studio (#7031) @status102
- 修复“迷城”界面公开招募有时出错的问题 @zzyyyl
- 修复肉鸽探索失败后卡在每月委托/解锁收藏品等界面 @zzyyyl
- 繁中服 保全派駐識別修正 (#7041) @vonnoq
- 剪切板错误 @ABA2396
- fix #7027 (#7039) @Cryolitia
- 修复肉鸽打完退出的问题 @zzyyyl
- 修复Wpf自动战斗-战斗列表-批量导入时，缓存文件夹未存在导致报错的bug；调整关卡名匹配规则 (#6988) @status102
- YostarEN Mizuki I.S. Trader + encouter regex (#6993) @Constrat
- 修复部分模板图片 @zzyyyl
- 修复美服 “A Cold Separation”（寒渊惜别）关卡识别错误 @zzyyyl
- 修复 maatouch 滑动 @zzyyyl
- 調整繁中服某些不期而遇的 ocrReplace (#6977) @momomochi987 @MistEO

### 其他

- add 1 sec delay for resource OTA @MistEO
- 增加 TaskSorter 脚本对 tasks.json 进行排序 (#7046) @zzyyyl
- Change to Appimage Package on Linux (#7030) @Cryolitia
- 删除多余引用 @ABA2396
- Auto Update Game Resources - 2023-10-22 @MistEO
- Auto Update Game Resources - 2023-10-22 @MistEO
- Auto Update Game Resources - 2023-10-19 @MistEO
- Revert "feat: 删除无用的 Roguelike@Continue 相关任务" @zzyyyl
- 补充基建排班协议中`period`字段不存在时的计划切换动作 (#6998) @status102
- re-write changelog_generator in algorithm (#6972) @quyansiyuanwang
