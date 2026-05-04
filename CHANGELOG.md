## v6.9.1

### 新增 | New

* 增加使用完整包更新时的强制提示，安装在根目录时禁止启动与更新 (#16435) @ABA2396
* pc 跳过数据上报 @ABA2396

### 改进 | Improved

* 重构战斗失败识别, 顺带支持沙盘战斗结束 (#16449) @status102
* 给WSA也加个 @status102
* 自动编队Elite 图标匹配失败时认为elite = 0 @status102
* 理智药到期时间OCR高度增加 @status102
* 使用临期药品时, 如果存在时效识别失败的药品则取消本次吃药 @status102

### 修复 | Fix

* 理智药识别支持3位数库存 @status102
* 仅在应用宝连接时额外指定InstanceOption级的ClientType @status102
* 绿票商店稳定性提升 (#16369) @Roland125
* 调整信用商店识别区域，修复 4 位数信用识别问题 @ABA2396
* 编队时无精英化的干员练度会被当成 <-1, -1> @ABA2396
* 修改recruit_one的返回值，适配更精细的招募计数(#16355) (#16371) @Roland125
* 修复剿灭结算时，识别不到合成玉基线 (#16460) @Roland125
* 肉鸽投资存款检测校验数据 @status102
* 更新剿灭入口的图片资源 (#16458) @Roland125 @Saratoga-Official
* 凯尔希识别 @Saratoga-Official
* 修复设置指引-连接设置-每次重新检测的提示块隐藏错误 @ABA2396
* typo @status102
* 自动战斗多作业模式导航retry_time @status102
* PixelAnalyzer::set_gray_ub 复制粘贴笔误写到了 m_ub (cv::Scalar) 而非 m_gray_ub @FireflySentinel
* RoguelikeRecruitSupportAnalyzer::analyze HH:MM:SS substr 偏移导致 hour 永为 0、sec 严重低估 ```cpp boost::regex_search(result.text, match_results, boost::regex("[0-9]{2}:[0-9]{2}:[0-9]{2}")) const auto& match_str = match_results[0].str();              // 8 字节 "HH:MM:SS" const auto& hour = std::atoi(match_str.substr(2).c_str());   // ":34:56" → 0 const auto& min  = std::atoi(match_str.substr(3, 2).c_str());// "34" ✓（碰巧） const auto& sec  = std::atoi(match_str.substr(7, 2).c_str());// "6"   → 个位 ``` @FireflySentinel
* RoguelikeFoldartalGainTaskPlugin::gain_stage_award `||` 链恒为 true 导致装备误入密文板列表; correct_rect计算顺序 @FireflySentinel
* correct_rect在rect负方向完全越界时依旧输出非0宽度结果 @status102
* GALLUS²识别 @Saratoga-Official
* 临期理智药使用输出遗漏时间 @status102
* 理智药剩余日期识别前缀非数字字符移除 @status102
* 移除不再生效的日服理智药roi覆盖 @status102
* 开始唤醒过早开始切换账号 @status102
* 部分场景下无法自动启动游戏 (#16422) @0x1b2c

### 文档 | Docs

* 补全连接阶段的 ClientType 参数说明 @ABA2396

### 其他 | Other

* rename @status102
* remove debug code @status102
