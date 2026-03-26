---
order: 7
icon: ic:round-translate
---

# 文档国际化 (i18n) 指南

本文档介绍如何使用 AI 翻译脚本将简体中文文档同步到其他语言版本。

::: tip
目前仅支持简体中文页面编辑后，使用AI 翻译脚本进行**单向**翻译和同步。

本文，即**文档国际化 (i18n) 指南**，是被翻译脚本**排除在外**的。
:::

## 前置准备

### 1. 安装依赖

```bash
cd docs
pnpm install
```

### 2. 配置 API 密钥

复制配置模板并编辑：

```bash
cp .env.example .env
```

编辑 `.env` 文件，填入你的 API 密钥：

```env
# OpenAI 兼容 API 配置（以DeepSeek api为例）
OPENAI_API_KEY=your-api-key-here
OPENAI_BASE_URL=https://api.deepseek.com/v1
OPENAI_MODEL=deepseek-chat

```
::: warning
翻译完毕之后，请记得把`.env`中的api取下，不然你的Token就有可能悄悄溜走啦！
:::

## 工作流程

### 步骤 1：检查同步状态

```bash
pnpm i18n:check
```

此命令会扫描 `zh-cn` 目录，对比其他语言文件的修改时间，显示哪些文件需要同步。

### 步骤 2：查看 Git 变更（可选）

如果你刚修改了文档，可以查看本次修改涉及哪些文件：

```bash
pnpm i18n:git
```

### 步骤 3：AI 自动翻译

翻译特定语言的所有文件：

```bash
# 翻译成英文
pnpm i18n:translate en-us

# 翻译成繁体中文
pnpm i18n:translate zh-tw

# 翻译成日语
pnpm i18n:translate ja-jp

# 翻译成朝鲜语
pnpm i18n:translate ko-kr
```

翻译特定目录：

```bash
pnpm i18n:translate en-us "develop/*.md"
pnpm i18n:translate ja-jp "manual/**/*.md"
```

### 步骤 4：初始化缺失文件

如果某个语言缺少文件，可以快速创建初始版本：

```bash
pnpm i18n:init
```

## 术语表

术语表位于 `tools/i18n/glossary.json`，用于统一专业术语的翻译。

添加新术语的格式：

```json
{
  "中文术语": {
    "en-us": "English Term",
    "zh-tw": "繁體術語",
    "ja-jp": "日本語用語",
    "ko-kr": "한국어 용어"
  }
}
```

## 注意事项

### 1. 翻译质量

AI 翻译虽然高效，但仍建议：
- 翻译后通读一遍，确保技术术语准确
- 检查链接中的语言代码是否正确替换（如 `/zh-cn/` → `/en-us/`）
- 代码块中的注释可能需要手动调整

### 2. Frontmatter 处理

翻译脚本会保留 frontmatter 结构，但以下内容不会自动翻译：
- `icon` 字段（图标名称）
- `order` 字段（排序数字）
- `date` 字段（日期）

### 3. 特殊标记

AI 翻译后的文件会自动保留原始 frontmatter，你可以在翻译后添加审校标记：

```markdown
---
order: 1
icon: jam:write-f
# translator: AI (DeepSeek-chat)
# reviewer: pending
---
```

## 命令速查

| 命令 | 功能 |
|------|------|
| `pnpm i18n:check` | 检查所有语言的同步状态 |
| `pnpm i18n:git` | 查看 Git 变更涉及的文件 |
| `pnpm i18n:init` | 为缺失的文件创建初始版本 |
| `pnpm i18n:translate <lang> [pattern]` | AI 翻译指定语言 |

## 常见问题

### Q: 翻译后文件格式错乱？

A: 请确保：
1. 源文件使用 UTF-8 编码
2. Markdown 语法正确（特别是 frontmatter 的 `---` 分隔符）
3. 代码块使用正确的围栏标记

### Q: 如何只翻译一个文件？

A: 使用具体文件路径作为 pattern：

```bash
node tools/i18n/translate.mjs en-us "develop/documentation-guidelines.md"
```

### Q: API 调用失败怎么办？

A: 检查以下几点：
1. API 密钥是否正确
2. 账户余额是否充足
3. 网络连接是否正常
4. 如使用国内服务，BASE_URL 是否配置正确

### Q: 如何批量翻译所有语言？

A: 可以使用 shell 循环：

```bash
for lang in en-us zh-tw ja-jp ko-kr; do
  pnpm i18n:translate $lang
done
```

## 贡献术语表

如果你发现术语翻译不准确，欢迎向 `tools/i18n/glossary.json` 提交 PR，帮助改进翻译质量。