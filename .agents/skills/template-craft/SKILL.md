---
name: template-craft
description: 制作与调整 MAA 任务的识别资源：截模板图/抠模板、定位 roi/定坐标/量坐标、取色生成 ColorMatch 参数、调 maskRange/mask_ranges/掩码范围。用户提到截模板/裁模板/模板图、roi 坐标、取色、ColorMatch、给新活动或新界面做图像适配，或点名 ImageCropper/MaskRangeTool 时使用，即使用户没有明确说出工具名。
---

# 模板与识别资源制作（ImageCropper / MaskRangeTool）

两个工具、分工如下：

| 工具 | 用途 | agent 入口 |
| --- | --- | --- |
| `tools/ImageCropper/` | 截模板、定 roi、取色 | CLI `cli.py`（无头）；GUI `main.py` 留给人拖框 |
| `tools/MaskRangeTool/` | maskRange 调参（LUV/HSV/RGB） | 直接 `import mask_utils` 调函数 |

ImageCropper 的 GUI 快捷键与设备截图（ADB / PC 窗口 WGC）见 `tools/ImageCropper/README.md`；CLI 子命令速查同见该 README 末尾。

## 前置

- **源图**：用户提供任意分辨率的截图文件路径即可，CLI 自动等比标准化；没有现成截图时用 GUI 连设备或截 PC 窗口（见 ImageCropper README，需人操作）。
- **依赖**：`tools/ImageCropper/` 与 `tools/MaskRangeTool/` 各有一份 `requirements.txt`。CLI 只需 opencv-python 与 numpy（MaskRangeTool 另需 matplotlib）；MaaFw 仅 GUI 连设备用，无头调用不必安装。

## 坐标系（先于一切）

- CLI 所有坐标基于 ｢等比标准化到短边 720｣ 的图像，与 GUI 框选输出、MAA 运行时 roi 同一空间。`grid` 的输出行会打印实际尺寸，先看它再读数（`crop`/`color` 不打印尺寸）。
- `--raw` 跳过标准化保持输入原分辨率。**对裁剪产物等小图做二次 grid/crop 必须加 `--raw`**，否则小图被放大、坐标系改变（CLI 会打印警告）。
- roi 格式 `x,y,w,h`；`--box` 接受 grid 读数习惯的两点式 `x1,y1,x2,y2`。
- **产物位置**：默认写 `tools/ImageCropper/dst/`——`crop` 产物为 `<stem>_<x>_<y>_<w>_<h>.png`、`grid` 为 `<stem>_grid.png`；stdout 会打印绝对路径，建议统一 `-o` 指到本次工作目录便于管理。

## 工作流（五步，档位按目标尺寸自适应）

1. **粗格定带**：`python cli.py <源图> grid --cell 40 -o <工作目录>/coarse.png`，读边缘刻度定位目标所在区域带。粗格对 20px 级小目标的读数误差可达 ±20px 甚至偏整格——只用来定 ｢哪一带｣ ，后续开窗每边留 6~10px 余量。**开窗前先辨认目标身份**：同排的文字块与圆形图标在粗格图上都是色块，身份错认会废掉整个首读。
2. **细格首读得候选 roi**：小目标（十几 px 级）可从粗格直跳 `--cell 1`（`--zoom 16+` 小窗），首读误差 0~3px；其余尺寸首档 cell 取目标短边的 1/5~1/10、逐级约 ÷4 收窄至 cell 1（中格档精度介于粗格 ±20px 与 cell=1 的 0~3px 之间），roi 本有容差，像素级只有模板裁剪才需要。
3. **标定窗回读**：按候选 roi 裁已知原点的略大窗（每边多 4~8px）：`python cli.py <源图> crop x,y,w,h -o <工作目录>`，再对产物 `python cli.py <产物>.png grid --raw --cell 1 --zoom 8 -o <工作目录>/calib.png`，Read 带刻度图量四边留白，反推真实边界。**无歧义的边（纯白背景）在此一步定死**——量留白比在放大图上读绝对坐标可靠得多。
4. **1px 窄带边缘裁决**：仅对残留歧义边（抗锯齿淡灰、材料边缘光），对候选 roi 外扩 3~6px 的小窗开 `--zoom 24` 以上 `--cell 1` 细格，逐行/列裁决归属。模板内部没有裁决量，不要全程 cell=1 扫描。**低对比目标（浅灰边框 vs 白底等）目测不可靠——高倍下仍会数错 1 列，应改用 `color` 子命令对边界行/列读色值（如传 1px 高的 roi，注意 roi 至少 3 像素），以数值阈值（如 RGB ≤240 算目标、更淡的抗锯齿残渣不算）裁决**，视觉上接近白的浅灰与纯白不可分，数值阈值才能把口径钉死。
5. **终裁 + 双证据回读验证**：按最终 roi 裁剪 `python cli.py <源图> crop x,y,w,h --amp -o <工作目录>`，另对 (x−2, y−2, w+4, h+4) 裁一张对照窗：原产物四边应恰为目标边界、对照图中外扩出的 2px 环带应全为背景；环带内出现目标内容即判定切边，回第 4 步。小裁剪产物在 Read 中被插值放大，｢刚好紧致｣ 与 ｢裁掉了｣ 难辨，双证据缺一不可。

**交付边界**：本工作流止于产出最终模板图与 roi/ColorMatch/maskRange 数值，默认将产物绝对路径与数值整理回报给用户；写入 `resource/template/` 子目录与 `resource/tasks/*.json` 属任务集成步骤，由调用场景另行决定。

## 精度原则

- **Read 展示端可能缩放图片，读数只认基准线，禁止格内占比插值**。grid 图经 Read 工具展示时可能被放大或缩小，展示比例未知且不可控，因此 ｢目标在格内占了几分之几｣ 的目测（如 0~40 一格 ｢看着过半｣ 就报 >20 ）完全不可靠。唯一合法读数：目标边缘对齐哪根带数字的基准线就报那根线的值（数字起始位置=线位），输出永远是线上的整值；线以下的精度需求交给标定窗回读量留白，不靠肉眼细分。
- **模板必须纯净**：内容充满画面、无背景残边，换场景才匹配得上；容差是 roi 的属性，不是模板的。
- 视觉读 grid 的可靠精度：粗格 ±20px 且可偏整格；cell=1 首读 0~3px，字形复杂处可能整段看漏（误差直达包络上界）；像素级定界以标定窗回读为准，细格读数只当起点。
- **口径显式统一**：边缘的淡灰抗锯齿/边缘光按数值阈值定义归属（判据见工作流第 4 步，如 RGB ≤240 算目标）还是仅实心，选定后四边一致——肉眼口径会在不同边之间漂移，产生 ±1~3px 的假误差。
- **先验会反向干扰**：｢圆应宽高相等｣ ｢字应有某笔画｣ 与图面证据冲突时以图为准——目标的渲染特征（如边缘光分布不对称）可使外接矩形不满足几何先验。
- zoom 16 判不住最淡边缘，终裁用 zoom 24+ 复核。
- **黄细线会给淡色像素染色**：低倍下白底上的淡 halo 可能被 1px 线污染读成着色、高倍下才可分辨，边缘归属要在单一最高倍窗一次裁定，不混用多档 zoom 的读数。
- **强柱旁的弱渐变柱易被并柱漏读**：圆弧最远端等处紧贴强边框柱的弱渐变列，视觉读数易整体漏掉——终裁前用 `color` 子命令对候选边界外 1~2 列/行读色值复核。
- **亚像素渲染会让四边灰带宽度不一致**（各边的实/淡列数不必相同），按 ｢灰带外缘｣ 目测会使四边口径各自漂移；低对比目标按数值阈值裁决（见工作流第 4 步）。
- **收敛复验必须含原点**：first_read 与 final 可能同尺寸不同原点，只对 w/h 不对 x/y 会漏掉误差。

## ColorMatch 取色

`color` 子命令对 roi 聚类主色，输出可直接并入任务 JSON 的草稿（`recognition`/`roi`/`method`/`lower`/`upper`/`count`/`connected`，method 4 = RGB 空间）。`--connected` 要求命中连成片（实心色块状态判断用），默认按像素计数。GUI 对应 C/c 键，但 GUI 仅输出首个聚类且向量为单层展开，与 CLI 草稿（全部聚类、完整三通道向量）结构不同，勿交叉对照。

## MaskRangeTool

`main.py` 是写死示例路径的演示脚本，agent 直接调 `mask_utils` 函数：

```python
import sys
sys.path.insert(0, r"<仓库根>\tools\MaskRangeTool")
import cv2
from mask_utils import generate_mask_ranges, compare_2_image_with_mask_ranges

img = cv2.imread("<模板图>.png")
# 自动推荐 maskRanges；show=False 无头拿返回值，save 存直方图+mask 预览供回读复核
ranges = generate_mask_ranges(img, "luv", base_mask_ranges, show=False, save="preview.png")
# 两组图在给定 ranges 下的差异对比，验证 maskRange 的区分度
compare_2_image_with_mask_ranges(img1, img2, ranges, "luv", show=False, save="cmp.png")
```

忽略亮/暗背景的 base_mask_ranges 预设见 `main.py` 顶部；参数 `color` 支持 luv/hsv/rgb。

## 常见错误

- ❌ 在格内按占比插值读数（如 ｢0~40 格里过半，报 >20 ｣）——Read 展示端可能缩放图片，比例目测不可靠；cell=1 读数应从最近基准线逐根黄线数到目标边缘，只报对齐基准线的整值。
- ❌ 对裁剪产物/小图 grid 忘 `--raw`（被标准化放大，坐标系失真）。
- ❌ 模板带背景或残边就交付（换个场景匹配不上）；容差写在 roi 里，不靠模板带余量。
- ❌ 全程 cell=1 扫描大区域（内部无裁决量，浪费且无收益）。
- ❌ 四条边淡灰口径不一致，或凭 ｢圆应该对称｣ 修改图面读数。
- ❌ 只看单张小裁剪产物就判定紧致（插值放大不可辨，需外扩对照窗）。
- ❌ 粗定位开窗贴着估计值（应每边放宽 6~10px，粗格小目标误差可达一格）。
