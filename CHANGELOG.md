## v4.10.0-alpha.2

- 初步支持 保全派驻 抄作业功能 @MistEO
  目前 bug 非常多，仅供尝鲜，基本只能打前三层。且暂不支持自动编队，请手动编队完成后于 “开始部署” 界面开始任务。请期待后续优化
- 重构 干员头像缓存机制 @MistEO
- 优化 `引航者试炼` 移动镜头动作，无需再手动设置延迟 @MistEO
- 优化 资源加载速度，修复资源加载时崩溃的问题 @MistEO
- 修复 界面 关卡选择为 null 时的崩溃问题 @ABA2396
- 优化 界面 自定义基建日志打印 @MistEO

## v4.10.0-alpha.1

- 新增 `引航者试炼` 抄作业功能支持，新增 `MoveCamera` 字段以移动镜头 @MistEO @horror-proton  
  请参考 [战斗流程文档](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/3.3-%E6%88%98%E6%96%97%E6%B5%81%E7%A8%8B%E5%8D%8F%E8%AE%AE.md) 使用
- 新增 界面 新版本下载进度显示 @moomiji
- 新增 界面 活动剩余时间提示 @ABA2396
- 优化 资源加载 速度 @dantmnf @MistEO
- 重构 界面 关卡导航列表从 web 获取 @MistEO @ABA2396
- 重写 掉落识别/仓库识别 数量检测算法，暂时关闭上传企鹅功能 @horror-proton @MistEO
- 修复 自动战斗/肉鸽 不停点 CD 干员的问题 @MistEO
- 修复 自动战斗/肉鸽 偶现干员识别错误 @MistEO
- 修复 肉鸽 助战功能 翻页后不识别的问题 @WLLEGit
- 更新 自定义基建 内置作业界面 @ABA2396
- 更新 文档，更新 加群链接 @MistEO @ntgmc @DavidWang19

### For overseas

- Added translations for drop settings @ABA2396

### For develops

- Optimized the Rust interface @KevinT3Hu
- Updated Linux tutorial @horror-proton
