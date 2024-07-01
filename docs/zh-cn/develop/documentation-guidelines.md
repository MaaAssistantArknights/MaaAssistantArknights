---
order: 6
icon: jam:write-f
---

# 文档编写指南

::: tip
本文档的目的在于指导文档编写者更好的使用主题提供的功能，以此达到更易读的效果。
:::

我们的文档基于 [vuepress](https://github.com/vuejs/vuepress) 构建，使用了 [vuepress-theme-hope](https://github.com/vuepress-theme-hope/vuepress-theme-hope) 主题，你也可以查看[官方文档](https://theme-hope.vuejs.press/zh/)来获取更加详细的说明，这里仅介绍一些常用的功能

## 本地部署

1. 安装 [pnpm](https://pnpm.io/installation)，并参考 [Pull Request 流程简述](./development.md#github-pull-request-流程简述)将仓库克隆到本地。
2. 在 `website` 目录下新建终端，运行 `pnpm i` 部署依赖。
3. 运行 `pnpm run dev` 进行部署。

## 容器

~~不是 docker 那个容器~~

该主题提供了关于提示、注释、信息、注意、警告和详情自定义容器的支持，我们可以利用这一特性来强调部分内容

容器的使用方法：

```markdown
::: [容器类型] [容器标题（可选）]
你想写的内容
:::
```

接受的容器内容与其默认标题如下：

- `tip` 提示
- `note` 注释
- `info` 信息
- `warning` 注意
- `danger` 警告
- `details` 详情

### 容器示例

::: tip
这是提示容器
:::

::: note
这是注释容器
:::

::: info
这是信息容器
:::

::: warning
这是注意容器
:::

::: danger
这是危险容器
:::

::: details
这是详情容器
:::

## 图标

该主题提供了图标支持，你可以在以下地方使用图标:

- 文档标题: 在 frontmatter 中设置文档标题旁边的图标。

- 导航栏/侧边栏: 设置在导航栏与侧边栏中显示的图标。

- 文档内容: 在文档中使用图标。

### 设置文档的图标

你可以在文档的 [frontmatter](#frontmatter) 中使用 `icon` 来设置文档的图标。

这个图标会显示在文档标题的旁边。

::: details 本文档的 frontmatter 设置

```markdown
---
icon: jam:write-f
---
```

:::

### 在文档中使用图标

你可以使用 `<HopeIcon />` 组件在 markdown 中添加图标。该组件有以下属性：

- `icon` 接受图标关键字及 Url，如 `jam:write-f`，`ic:round-home` 等
- `color` 接受 css 风格的颜色值，如 `#fff`，`red` 等（该选项仅对 svg 图标有效）
- `size` 接受 css 风格的大小，如 `1rem`，`2em`，`100px` 等

::: details 案例
<HopeIcon icon="ic:round-home" color="#1f1e33"/>

```markdown
<HopeIcon icon="ic:round-home" color="#1f1e33"/>
```

<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```markdown
<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```
:::

### 图标关键字的获取

本文档使用的图标来自于 [iconify](https://iconify.design/)，你可以在其给出的 [图标搜索界面](https://icon-sets.iconify.design/) 中搜索你想要的图标，然后复制其关键字。

## Frontmatter

Frontmatter 是 Markdown 文档开头一段用 `---` 包裹起来的内容，其内部使用 yml 语法。通过 Frontmatter，我们可以标识文档的编辑时间，使用的图标，分类，标签等等。

::: details 示例

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# 文档标题

...
```

:::

各字段含义如下：

- `date`：文档的编辑时间
- `icon`：文档标题旁边的图标
- `order`：文档在侧边栏中的排序
