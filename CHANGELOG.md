## v4.23.3

### 新增

- 添加CLI支持 (#6144) @MistEO @wangl-cc
   - add spaces @MistEO
   - add space @MistEO
   - fix install dir and less line break @wangl-cc
   - revert remove trailing whitespace @wangl-cc
   - format @wangl-cc
   - remove unrelevant changes @wangl-cc
   - fix download cli @wangl-cc
   - space between ZH and EN @wangl-cc
   - more details about CLI installation @wangl-cc
   - ship CLI in Linux release @wangl-cc
   - add CLI doc @wangl-cc
- shutdown/hibernate after all other MAAs have exited, or just ex… (#6147) @Neptsun
   - 日文localization写反了 @Neptsun
   - shutdown/hibernate after all other MAAs have exited, or just exit itself @Neptsun
- 支持woolang绑定 (#6142) @mrcino
   - * docs: 补充Woolang-api的说明 @mrcino
   - * feat: 支持woolang绑定，嘻嘻嘻 @mrcino
- 注册表查询bluestacks hyper-v的conf位置 (#6083) @Gliese129
   - optimized the way of testing adb port @Gliese129
   - update @Gliese129
   - use register key to detect bs config @Gliese129
   - add default bluestacks.conf @Gliese129
   - 多个模拟器可能导致连接不上 @Gliese129
- 增加刷理智每次打关卡之前时的理智剩余量输出 (#6146) @status102
   - 修复在部分情况下只会输出一次当前理智情况 @status102
   - 补上丢失的空格 @status102
   - 移除无用内容 @status102
   - 增加刷理智每次打关卡之前时理智剩余量输出 @status102

### 改进

- 什么时候更新 .net8 什么时候加.jpg @ABA2396
- 更新提醒 @ABA2396

### 修复

- 规避crt死锁问题 @MistEO
- 尝试修复LoadResource卡死的问题 @MistEO
- 修复理智识别在部分情况下会"/"识别为"|" (#6170) @status102
   - remove test code @status102
   - 修复理智识别在部分情况下会"/"识别为"|" @status102

### 其他

- Merge tag 'v4.23.2' into dev @MistEO
- Update CHANGELOG.md @MistEO
- Update localization for YoStarKR @178619
- Update ocrReplaces for YoStarKR @178619
- 跟进蓝叠Hyper-V的连接指导 (#6083) (#6137) @MistEO @SherkeyXD
   - add space @MistEO
   - 跟进蓝叠Hyper-V的连接指导 @SherkeyXD
- 取消连接时加载资源 @ABA2396
- 加点加载资源和连接模拟器的日志 @ABA2396
- Merge tag 'v4.23.1' into dev @ABA2396
- Auto Update Game Resources - 2023-08-31 @MistEO
- i18n: Translations update from MAA Weblate (#6151) @weblate
   - Translated using Weblate (Japanese) @weblate
- add log @AnnAngela
- Auto Update Game Resources - 2023-08-31 @MistEO
