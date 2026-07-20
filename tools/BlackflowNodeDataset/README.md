# 黑流树海节点数据集辅助工具

默认运行：

```powershell
python tools/BlackflowNodeDataset/cut_node_crops.py
```

脚本默认读取：

```text
C:\Users\10067\OneDrive\文档\MuMu共享文件夹\Screenshots\map\fin
```

并输出到同目录的 `node_dataset`：

- `_unclassified`：检测到的对象节点，人工移动到对应类型目录。
- `_road`：自动检测为道路的节点。
- `previews`：原图加节点框和编号，方便定位来源。
- `manifest.json`：小图、原图、网格坐标和裁剪框的对应关系。
- 类型目录：根据 `resource/roguelike/Blackflow/map.json` 自动创建为空目录。

目前使用现有的无 OCR 地图提取器定位网格和节点；分类标签完全由人工移动文件决定。
训练出的 `Blackflow_node_type.onnx` 已接入 C++ 地图识别器，替代黑流树海节点铭牌 OCR。

训练节点类型分类器：

```powershell
python tools/BlackflowNodeDataset/train_node_classifier.py
```

训练脚本会先输出类别检查和两种验证结果（随机切分、按截图来源切分），再将最终模型保存到
`node_dataset/model/Blackflow_node_type.onnx`。当前模型使用的是归一化后的图像特征，
尚未接入 C++ 地图分析器。

查看验证集误分类样本：

```powershell
python tools/BlackflowNodeDataset/inspect_errors.py
```

结果会写入模型目录的 `errors_random_holdout.png`、`errors_screenshot_group_holdout.png`
和 `errors.json`。
