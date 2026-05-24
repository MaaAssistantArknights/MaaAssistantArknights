---
name: code-review-report
description: >-
  全项目代码审查流水线，输出结构化审查报告到 Markdown 文件。
  按模块拆分 Review Unit，派发 subagent 并行审查，汇总发现并分类，
  生成完整的问题清单报告，不执行任何修复。
  Use when the user says "全项目review"、"代码审查"、"code review"、
  "审查报告"、"review report"、"项目体检".
disable-model-invocation: true
---

# Code Review Report Pipeline

全项目代码审查 → 并行 Review → 汇总发现 → 生成报告（只审不修）。

## 项目背景

MAA (MaaAssistantArknights) 是一个多语言 monorepo，包含核心引擎、多端 GUI、CLI、语言绑定等子项目：

### 核心 & GUI

| 模块 | 语言 | 路径 | 子模块? | 构建 |
|------|------|------|---------|------|
| MaaCore | C++20 | `src/MaaCore/` | 否 | CMake |
| MaaWpfGui | C# / WPF (.NET 10) | `src/MaaWpfGui/` | 否 | MSBuild |
| MaaMacGui | Swift / SwiftUI | `src/MaaMacGui/` | **是** | Xcode |
| MAAUnified | C# / Avalonia (.NET 10) | `src/MAAUnified/` | **是** | dotnet |
| maa-cli | Rust | `src/maa-cli/` | **是** | Cargo |
| MaaUtils | C++ | `src/MaaUtils/` | **是** | CMake |
| MaaWineBridge | C | `src/MaaWineBridge/` | 否 | CMake |
| MaaUpdater | C++ (Win) | `src/MaaUpdater/` | 否 | CMake |

### 语言绑定 & 工具

| 模块 | 语言 | 路径 | 说明 |
|------|------|------|------|
| Python 绑定 | Python | `src/Python/` | ctypes FFI |
| Rust 绑定 | Rust | `src/Rust/` | FFI + HTTP server |
| Go 绑定 | Go 1.23 | `src/Golang/` | Gin HTTP wrapper |
| Java 绑定 | Kotlin/Java | `src/Java/` | JNA + Ktor HTTP/WS |
| Dart 绑定 | Dart | `src/Dart/` | Flutter FFI plugin |
| Woolang 绑定 | Woolang | `src/Woolang/` | C API wrapper |
| C++ 示例 | C++ | `src/Cpp/` | 集成示例 |
| 工具脚本 | Python/C++/Shell | `tools/` | 开发维护工具 |
| 任务资源 | JSON | `resource/` | 任务定义/模板/OCR |
| 文档站 | Markdown/TS | `docs/` | VuePress |
| 公共头文件 | C | `include/` | AsstCaller.h 等 |

### 编码规范执行

| 配置文件 | 作用范围 |
|----------|----------|
| `.clang-format` | C++ (pre-commit 限定 `src/MaaCore/**`) |
| `.editorconfig` (多层) | 全局 + MaaCore/MaaWpfGui/maa-cli/MaaUtils 各有覆盖 |
| `rustfmt.toml` | Rust (maa-cli) |
| `.swift-format` | Swift (MaaMacGui) |
| `stylecop.json` | C# (MaaWpfGui) |
| `.prettierrc` | JSON/YAML |
| `analysis_options.yaml` | Dart |
| `.pre-commit-config.yaml` | clang-format + Prettier + Ruff + markdownlint |

## Phase 1: 探索 & 拆分 Review Unit

1. 用 `explore` subagent 扫描项目，确认当前有哪些模块/子目录有实质改动或需要关注
2. 按 **模块 × 关注维度** 拆分为 12-20 个 Review Unit，每个 Unit：
   - 文件范围 ≤ 8 个核心文件（C++ 可适当放宽，但避免单 Unit 超 2000 行总量）
   - 有明确的 review 焦点
   - 提供该模块的背景信息
3. 标注优先级：P0（安全/崩溃/数据损坏）、P1（可靠性/性能/兼容性）、P2（代码质量/可维护性）

### 各语言 Review 焦点

**C++ (MaaCore / MaaUtils / MaaWineBridge / MaaUpdater)**：

- 内存安全（裸指针、RAII 遗漏、use-after-free）
- 线程安全（共享状态、锁粒度、竞态条件）
- 异常安全（析构器 throw、RAII 保证）
- 图像识别/OCR 流程正确性
- 跨平台兼容（Windows/Linux/macOS 条件编译）
- MaaWineBridge：Wine/native 转发正确性

**C# (MaaWpfGui)**：

- MVVM 模式遵循（View/ViewModel 职责划分）
- INotifyPropertyChanged 正确性
- UI 线程安全（Dispatcher 调用）
- 资源泄露（IDisposable）
- 本地化完整性
- StyleCop 规范合规

**C# / Avalonia (MAAUnified)**：

- 跨平台 UI 兼容（macOS/Linux/Windows）
- CoreBridge 与 MaaCore 交互正确性
- 分层架构合理性（App/Application/Platform/CoreBridge）
- 与 MaaWpfGui 功能对齐一致性

**Swift (MaaMacGui)**：

- SwiftUI 生命周期管理
- MaaCore FFI 调用安全性（指针/内存管理）
- macOS 平台特性使用（沙盒、权限）
- `.swift-format` 规范合规

**Rust (maa-cli)**：

- 所有权/生命周期正确性
- Error handling（`?` 传播链完整性）
- unsafe 代码合理性与安全注释
- Cargo feature 配置正确性
- 参考 `src/maa-cli/AGENTS.md` 的检查项

**语言绑定 (Python/Rust/Go/Java/Dart/Woolang)**：

- FFI 调用安全性（指针、回调、生命周期）
- 资源释放（Handle/Destroy 配对）
- 与 `include/AsstCaller.h` 公共 API 的一致性
- 错误传播与异常处理

**Python (tools/)**：

- 类型注解一致性
- 异常处理（不吞异常）
- 文件路径跨平台处理

**JSON (resource/)**：

- Schema 合规性
- 模板匹配参数合理性
- 多语言/多服务器资源一致性

## Phase 2: 并行 Review

按优先级批次启动 subagent：

```
第一批：P0 Unit（3-5 个并行）
第二批：P1 Unit（5-7 个并行）
第三批：P2 Unit（剩余全部）
```

每个 review subagent 的 prompt 模板：

```
你是 MAA 项目的代码审查员。审查以下文件，找出：
1. Bug（逻辑错误、边界条件、竞态、崩溃风险）
2. 安全问题（缓冲区溢出、注入、信息泄露、不安全的反序列化）
3. 性能问题（不必要的拷贝、内存分配热点、O(n²) 算法）
4. 跨平台兼容性（平台特定代码未条件编译、路径分隔符硬编码）
5. 可维护性（巨型函数、重复代码、缺少错误处理）

项目语言：{language}
文件范围：{files}
背景：{background}
重点关注：{focus_areas}

输出格式：按严重性排序的问题列表（最多 Top 8）。每个问题包含：
- 严重性：Critical / Major / Minor
- 位置：文件名 + 行号范围
- 问题：一句话描述
- 影响：会导致什么后果
- 建议：修复方向（一句话）
- 代码片段：相关代码（可选，简短引用即可）
```

## Phase 3: 汇总 & 生成报告

收集所有 Unit 的发现，按以下步骤生成报告：

1. **去重合并**：同一 bug 在多个 Unit 被发现时，保留最详细的描述
2. **分类归类**：

| 分类 | 含义 | 图标 |
|------|------|------|
| 崩溃/安全 | 可导致崩溃或被利用 | 🔴 |
| 可靠性 | 影响功能正确性 | 🟠 |
| 性能 | 影响运行效率 | 🟡 |
| 兼容性 | 跨平台/版本兼容问题 | 🔵 |
| 代码质量 | 可维护性与规范 | ⚪ |

1. **按模块和严重性排序**
2. **生成统计摘要**

### 报告模板

将报告输出到项目根目录的 `code-review-report.md`，使用以下模板：

```markdown
# MAA 代码审查报告

> 审查时间：{date}
> 审查范围：{modules_reviewed}
> Review Unit 数量：{unit_count}

## 摘要

| 严重性 | 数量 |
|--------|------|
| 🔴 Critical | {n} |
| 🟠 Major | {n} |
| 🟡 Minor | {n} |
| 总计 | {total} |

| 模块 | Critical | Major | Minor |
|------|----------|-------|-------|
| MaaCore (C++) | {n} | {n} | {n} |
| MaaWpfGui (C#) | {n} | {n} | {n} |
| MaaMacGui (Swift) | {n} | {n} | {n} |
| MAAUnified (C#/Avalonia) | {n} | {n} | {n} |
| maa-cli (Rust) | {n} | {n} | {n} |
| MaaUtils (C++) | {n} | {n} | {n} |
| MaaWineBridge (C) | {n} | {n} | {n} |
| 语言绑定 | {n} | {n} | {n} |
| tools (Python) | {n} | {n} | {n} |
| resource (JSON) | {n} | {n} | {n} |

## 🔴 Critical 问题

### [{序号}] {问题标题}

- **模块**：{module}
- **文件**：`{file}:{line_range}`
- **分类**：{category}
- **描述**：{description}
- **影响**：{impact}
- **建议修复方向**：{suggestion}

{code_snippet（可选）}

---

## 🟠 Major 问题

### [{序号}] {问题标题}
...

## 🟡 Minor 问题

### [{序号}] {问题标题}
...

## 审查覆盖范围

| Review Unit | 模块 | 焦点 | 优先级 | 文件数 |
|-------------|------|------|--------|--------|
| {unit_name} | {module} | {focus} | {priority} | {file_count} |
| ... | ... | ... | ... | ... |

## 附注

- 本报告仅列出发现，未执行任何修复
- 建议按 Critical → Major → Minor 顺序处理
- 部分问题可能需要跨模块协同修复
```

## 执行要点

- **只审不修**：本 skill 不修改任何代码，所有发现仅记录到报告
- **报告路径**：默认输出到 `code-review-report.md`，用户可指定其他路径
- **增量 vs 全量**：如用户指定范围（如"只看 MaaCore"），相应缩减 Unit 拆分
- **subagent 并行上限**：每批不超过 7 个，避免上下文竞争
- **代码片段引用**：报告中引用代码时使用 ` ```startLine:endLine:filepath ` 格式
