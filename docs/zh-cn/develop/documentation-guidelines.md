---
order: 6
icon: jam:write-f
---

# 文档编写指南

::: tip
本文档的目的在于指导文档编写者更好的使用主题提供的功能，以此达到更易读的效果。
:::

我们的文档基于 [vuepress](https://github.com/vuejs/vuepress) 构建，使用了 [vuepress-theme-plume](https://github.com/pengzhanbo/vuepress-theme-plume) 主题，你也可以查看[官方文档](https://theme-plume.vuejs.press/)来获取更加详细的说明，这里仅介绍一些常用的功能，或是经过我们自定义的功能

## 本地部署

1. 安装 [pnpm](https://pnpm.io/installation)，并参考 [Pull Request 流程简述](./development.md#github-pull-request-流程简述)将仓库克隆到本地。
2. 在 `docs` 目录下新建终端，运行 `pnpm i` 部署依赖。
3. 运行 `pnpm run dev` 进行部署。

## 容器与卡片

该主题提供了关于提示、注释、信息、注意、警告和详情自定义容器的支持，我们可以利用这一特性来强调部分内容

容器的使用方法：

```markdown
::: [容器类型] [容器标题（可选）]
你想写的内容
:::
```

或是使用 GitHub 风格语法

```markdown
> [!容器类型]
> 你想写的内容
```

接受的容器内容与其默认标题如下：

- `tip` 提示
- `note` 注
- `info` 相关信息
- `warning` 注意
- `danger` 警告
- `details` 详情
- `demo-warpper` ==特殊容器==

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

::: demo-wrapper
这是一个很特殊的容器
:::

## 马克笔标记

你可以使用标记语法来对想要的内容进行标记，用于强调重点事项

使用方法：用 `==标记内容=={标记颜色（可选）}` 的语法进行标记，请注意两边需要有空格

**输入：**

```markdown
MaaAssistantArknights 是由 ==很多猪== 开发的
```

**输出：**

MaaAssistantArknights 是由 ==很多猪== 开发的

主题还内置了以下的配色方案：

- **default**: `==Default==` - ==Default==
- **info**: `==Info=={.info}` - ==Info=={.info}
- **note**: `==Note=={.note}` - ==Note=={.note}
- **tip**: `==Tip=={.tip}` - ==Tip=={.tip}
- **warning**: `==Warning=={.warning}` - ==Warning=={.warning}
- **danger**: `==Danger=={.danger}` - ==Danger=={.danger}
- **caution**: `==Caution=={.caution}` - ==Caution=={.caution}
- **important**: `==Important=={.important}` - ==Important=={.important}

## 隐藏文本

出于某种原因，你可能需要将文档的某部分暂时涂黑，在这种情况下你可以使用隐藏文本功能

你可以使用 `!!需要隐秘的内容!!{配置（可选）}` 的语法来使用，默认效果如下

!!总感觉在看萌百（划掉!!

有以下配置可以使用

::: demo-wrapper
输入：

```markdown
+ 遮罩层效果 + 鼠标悬停：!!鼠标悬停看到我了!!{.mask .hover}
+ 遮罩层效果 + 点击：!!点击看到我了!!{.mask .click}
+ 文本模糊效果 + 鼠标悬停：!!鼠标悬停看到我了!!{.blur .hover}
+ 文本模糊效果 + 点击：!!点击看到我了!!{.blur .click}
```

输出：

- 遮罩层效果 + 鼠标悬停：!!鼠标悬停看到我了!!{.mask .hover}
- 遮罩层效果 + 点击：!!点击看到我了!!{.mask .click}
- 文本模糊效果 + 鼠标悬停：!!鼠标悬停看到我了!!{.blur .hover}
- 文本模糊效果 + 点击：!!点击看到我了!!{.blur .click}

:::

## 步骤

当你正在写一个步骤化的教程时，有序列表可能会因为嵌套失去层次感，这种时候 `steps` 容器就是最好的选择

注意该容器用四个冒号来标记开始和结束，与常规的容器不同

输入：

````markdown
:::: steps
1. 步骤 1

   ```ts
   console.log('Hello World!')
   ```

2. 步骤 2

   这里是步骤 2 的相关内容

3. 步骤 3

   ::: tip
   提示容器
   :::

4. 结束
::::
````

输出：

:::: steps

1. 步骤 1

   ```ts
   console.log('Hello World!')
   ```

2. 步骤 2

   这里是步骤 2 的相关内容

3. 步骤 3

   ::: tip
   提示容器
   :::

4. 结束

::::

## 智能图片容器

我们基于主题提供的功能包装了一个图片容器。该容器能够在亮暗主题下自动显示对应主题的，同时支持自动布局

你可以在 markdown 正文中使用 `<ImageGrid>` 组件来调用该方法，具体的语法和效果如下

::: demo-wrapper

这是语法：

```markdown
<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  },
  {
    light: 'images/zh-cn/readme/3-light.png',
    dark: 'images/zh-cn/readme/3-dark.png'
  },
  {
    light: 'images/zh-cn/readme/4-light.png',
    dark: 'images/zh-cn/readme/4-dark.png'
  }
]" />
```

这是渲染效果：

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  }
]" />

:::

## 字段容器

该语法较为复杂，请参考 [官方文档](https://theme-plume.vuejs.press/guide/markdown/field/) 进行使用

效果展示如下

:::: field-group
::: field name="theme" type="ThemeConfig" required default="{ base: '/' }"
主题配置
:::

::: field name="enabled" type="boolean" optional default="true"
是否启用
:::

::: field name="callback" type="(...args: any[]) => void" optional default="() => {}"
<Badge type="tip" text="v1.0.0 新增"  />
回调函数
:::

::: field name="other" type="string" deprecated
<Badge type="danger" text="v0.9.0 弃用"  />
已弃用属性
:::
::::

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

你可以使用 `<Icon />` 组件在 markdown 中添加图标。该组件有以下属性：

- `icon` 接受图标关键字及 Url，如 `jam:write-f`，`ic:round-home` 等
- `color` 接受 css 风格的颜色值，如 `#fff`，`red` 等（该选项仅对 svg 图标有效）
- `size` 接受 css 风格的大小，如 `1rem`，`2em`，`100px` 等

::: demo-wrapper 案例

输入：

```markdown
- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />
```

输出：

- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />

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
