## v6.1.0-beta.2

### Highlights

#### 界园导游持续完善 | Roguelike IS6 Refinement

感谢各位玩家的反馈！
这个版本主要修复了界园肉鸽的遭遇识别和通宝/铜币兑换识别问题。
针对 EN/KR/JP 服务器优化了大量 OCR 正则表达式，提升遭遇节点与商店流程的稳定性。
如遇问题请通过 Discord 服务器或直接在 GitHub 上反馈给我们。

----

#### IS6 Encounter & Tongbao / Copper Detection Fixes

Thanks for your feedback!
This version focuses on fixing IS6 encounter recognition and Tongbao/Copper exchange detection issues.
Multiple OCR regex improvements for EN/KR/JP servers enhance stability in encounter nodes and shop flows.
Please continue reporting issues to us via the Discord Server or directly on GitHub.

----

### 新增 | New

* 满线索再一键置入 @ABA2396

### 修复 | Fix

* more tongbao EN regex @Constrat
* SendClues for txwy (#15178) @momomochi987
* 通宝识别失败时放弃通宝 (#15167) @travellerse @HX3N
* more Coppers EN regexes @Constrat
* missing Special Squad for EN @Constrat
* more regex for CoppersNameOcrReplace EN @Constrat
* SendClues for Yostar server (#15168) @HX3N
* Missing GetDropSelectRecruit for EN @Constrat
* missing CoppersAbandonExchange for EN @Constrat
* tonbgbao regex for EN @Constrat
* 完善通宝识别失败时的分支处理 (#15180) @travellerse

### 其他 | Other

* 繁中服保全派駐 8 清音安保派駐 (#15110) @momomochi987
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* YostarKR JieGarden Encounter ocr @HX3N
* 更新 macos.cmake (#15173) @Alan-Charred
* KR tweak InvestmentReach @HX3N
