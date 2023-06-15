## v4.19.2-beta.1

### 新增

- 随机展示简中文档中模拟器支持列表 (#5161) @bakashigure
   - 更新模拟器列表注释 @bakashigure
   - 更新文档 @bakashigure
   - 随机展示简中文档中模拟器支持列表 @bakashigure

### 改进

- 更新了简中和英文的ocr det模型, 以及英文的ocr rec模型 (#5156) @Plumess
   - Update en_US det inference.onnx by using onnx_optimizer @Plumess
   - Update en_US rec inference.onnx by using onnx_optimizer @Plumess
   - Update en_US rec inference.onnx by using onnx_optimizer @Plumess
   - Update ch_CN inference.onnx by using onnx_optimizer @Plumess
- 重训了国服, 日服, 台服和韩服的OCR模型 (#5151) @Plumess
   - 重训了国服, 日服, 台服和韩服的OCR模型 (#5142) @Plumess
      - 重训韩服识别模型 | Re-training OCR rec model of KR client @Plumess
      - 重训台服识别模型 | Re-training OCR rec model of TW client @Plumess
      - 重训国服识别模型 | Re-training OCR rec model of CN client @Plumess
      - 重训日服识别模型 | Re-training OCR rec model of JP client @Plumess
- 优化退出模拟器，添加日志 @ABA2396

### 修复

- 修复自动基建 bskill_man_exp4 技能错误 @MistEO
- 扩大roi范围，修复PlayCover无法开始唤醒 #5099 @ABA2396
- 修了两个typo (#5149) @kongwei981126
   - Update copilot.json @kongwei981126
- fix filename @horror-proton

### 其他

- Auto Update Game Resources - 2023-06-15 @MistEO
- Auto Update Game Resources - 2023-06-13 @MistEO
- 修正拼写错误的函数名 @ABA2396
- Update en-bug-report.yaml @ABA2396
- Update cn-bug-report.yaml @ABA2396
- Typo on en-us.xaml (#5130) @doquangminh28
   - Update en-us.xaml @doquangminh28
- Changed translation en-us.xaml @ABA2396
- Changed translation en-us.xaml (#5111) @Constrat
   - Changed translation en-us.xaml @Constrat
- 移除不需要的changelog @AnnAngela
- 删除多余mac完整包 @AnnAngela
- Do not force push tag @MistEO
