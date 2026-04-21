## v6.8.0-beta.2

### 新增 | New

* 线索交流时先移除所有放置的线索 (#16054) @travellerse
* 启动时判断版本是否一致 @ABA2396
* 新增吃指定天数过期的理智药 (#13849) @soundofautumn @status102
* 添加单元测试框架和验证角色分配算法的测试用例 (#16245) @lhhxxxxx
* AVD 截图增强的售后（文档、CI 变更等） (#16031) @satgo1546
* V0.2 新构建跨平台前端界面 MAAUnified (#16048) @Halo5082
* 配置存储支持条件优化 (#15850) @status102
* 界园肉鸽月度小队和深入调查 (#16271) @SherkeyXD

### 改进 | Improved

* 外部更新使用分离的 updater (#16326) @ABA2396
* 外部更新不再读配置 @ABA2396
* 简化更新代码 @ABA2396
* 涉及 dll 的更新使用外部更新 @ABA2396
* 重构更新逻辑，允许拖入指定名称的压缩包进行更新 (#16308) @ABA2396

### 修复 | Fix

* updater utf8 解析 @ABA2396
* 描述误导 @status102
* 基建开启设施无法保存 @ABA2396
* index 越界 @status102
* macOS PlayTools/SCK 几处小修正 (#16276) @FireflySentinel
* 干员库存识别返回错误id @status102
* baseList 无法编译的问题 (#16293) @Yi-Zh17

### 文档 | Docs

* add FAQ guidance for Windows Defender false positives (#16145) @Leo91314

### 其他 | Other

* 注释推错了 @status102
* 调整手动更新方法描述 @ABA2396
* 繁中服宿舍截圖 & 部分 OCR 內容 (#16298) @momomochi987
* KR UseExpireMedicineForActivity @HX3N
