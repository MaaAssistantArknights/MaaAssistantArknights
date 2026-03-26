---
order: 6
icon: 'jam:write-f'
---
# Documentation Writing Guide

::: tip
The purpose of this document is to guide documentation writers on how to better utilize the features provided by the theme to achieve a more readable effect.

The code for this documentation site is located at [MaaAssistantArknights/docs](https://github.com/cnzhq/MaaAssistantArknights/tree/dev-v2/docs), not in [maa-website](https://github.com/MaaAssistantArknights/maa-website).
:::
<!-- When you're eager to write documentation, have forked the correct repository, and see this comment in VSCode (or somewhere else), it means you're at least on the right track. Unlike a certain big-headed pig who cloned the documentation site repository and searched for an entire afternoon :-( -->

Our documentation is built on [vuepress](https://github.com/vuejs/vuepress) and uses the [vuepress-theme-plume](https://github.com/pengzhanbo/vuepress-theme-plume) theme. You can also refer to the [official documentation](https://theme-plume.vuejs.press/) for more detailed instructions. Here we only introduce some commonly used features or features that we have customized.

## Local Deployment

1. Install [pnpm](https://pnpm.io/installation), and refer to [Pull Request Process Overview](./development.md#github-pull-request-流程简述) to clone the repository locally;
2. Open a terminal in the `docs` directory and run `pnpm i` to install dependencies;
3. Run `pnpm run dev` to deploy.

## Containers and Cards

The theme provides support for custom containers for tips, notes, information, warnings, dangers, and details. We can use this feature to emphasize certain content.

How to use containers:

```markdown
::: [Container Type] [Container Title (Optional)]
Content you want to write
:::
```

Or use GitHub-flavored syntax

```markdown
> [!Container Type]
> Content you want to write
```

Accepted container types and their default titles are:

- `tip` Tip
- `note` Note
- `info` Information
- `warning` Warning
- `danger` Danger
- `details` Details
- `demo-warpper` ==Special Container==

### Container Examples

::: tip
This is a tip container
:::

::: note
This is a note container
:::

::: info
This is an information container
:::

::: warning
This is a warning container
:::

::: danger
This is a danger container
:::

::: details
This is a details container
:::

::: demo-wrapper
This is a very special container!! Mimicking macOS traffic light window!!
:::

## Highlighter Marker

You can use the marker syntax to highlight content you want to emphasize.

Usage: Use the syntax `==Marked Content=={Marker Color (Optional)}` for marking. Please note that spaces are required on both sides.

**Input:**

```markdown
MaaAssistantArknights is developed by ==many pigs==
```

**Output:**

MaaAssistantArknights is developed by ==many pigs==

The theme also has the following built-in color schemes:

- **default**: `==Default==` - ==Default==
- **info**: `==Info=={.info}` - ==Info=={.info}
- **note**: `==Note=={.note}` - ==Note=={.note}
- **tip**: `==Tip=={.tip}` - ==Tip=={.tip}
- **warning**: `==Warning=={.warning}` - ==Warning=={.warning}
- **danger**: `==Danger=={.danger}` - ==Danger=={.danger}
- **caution**: `==Caution=={.caution}` - ==Caution=={.caution}
- **important**: `==Important=={.important}` - ==Important=={.important}

## Hidden Text

For some reason, you might need to temporarily black out part of the document. In this case, you can use the hidden text feature.

You can use the syntax `!!Content to be hidden!!{Configuration (Optional)}` to use it. The default effect is as follows:

!!It always feels like reading Moegirlpedia (strikethrough!!

The following configurations are available:

::: demo-wrapper
Input:

```markdown
+ Mask effect + Hover: !!You can see me on hover!!{.mask .hover}
+ Mask effect + Click: !!You can see me on click!!{.mask .click}
+ Text blur effect + Hover: !!You can see me on hover!!{.blur .hover}
+ Text blur effect + Click: !!You can see me on click!!{.blur .click}
```

Output:

- Mask effect + Hover: !!You can see me on hover!!{.mask .hover}
- Mask effect + Click: !!You can see me on click!!{.mask .click}
- Text blur effect + Hover: !!You can see me on hover!!{.blur .hover}
- Text blur effect + Click: !!You can see me on click!!{.blur .click}

:::

## Steps

When you are writing a step-by-step tutorial, ordered lists might lose hierarchy due to nesting. In such cases, the `steps` container is the best choice.

Note that this container uses four colons to mark the beginning and end, different from regular containers.

Input:

````markdown
:::: steps
1. Step 1

   ```ts
   console.log('Hello World!')
   ```

2. Step 2

   This is the content related to Step 2

3. Step 3

   ::: tip
   Tip container
   :::

4. End
::::
````

Output:

:::: steps

1. Step 1

   ```ts
   console.log('Hello World!')
   ```

2. Step 2

   This is the content related to Step 2

3. Step 3

   ::: tip
   Tip container
   :::

4. End

::::

## Smart Image Container

We have wrapped an image container based on the features provided by the theme. This container can automatically display the corresponding theme under light/dark themes and supports automatic layout.

You can use the `<ImageGrid>` component in the markdown body to call this method. The specific syntax and effect are as follows:

::: demo-wrapper

This is the syntax:

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

This is the rendering effect:

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

## Field Container

This syntax is relatively complex. Please refer to the [official documentation](https://theme-plume.vuejs.press/guide/markdown/field/) for usage.

Effect demonstration is as follows:

:::: field-group
::: field name="theme" type="ThemeConfig" required default="{ base: '/' }"
Theme configuration
:::

::: field name="enabled" type="boolean" optional default="true"
Whether enabled
:::

::: field name="callback" type="(...args: any[]) => void" optional default="() => {}"
<Badge type="tip" text="v1.0.0 Added"  />
Callback function
:::

::: field name="other" type="string" deprecated
<Badge type="danger" text="v0.9.0 Deprecated"  />
Deprecated property
:::
::::

## Icons

The theme provides icon support. You can use icons in the following places:

- Document Title: Set the icon next to the document title in the frontmatter.

- Navigation Bar/Sidebar: Set the icon displayed in the navigation bar and sidebar.

- Document Content: Use icons within the document.

### Setting Document Icons

You can use `icon` in the document's [frontmatter](#frontmatter) to set the document's icon.

This icon will be displayed next to the document title.

::: details Frontmatter settings for this document

```markdown
---
icon: jam:write-f
---
```

:::

### Using Icons in Documents

You can use the `<Icon />` component to add icons in markdown. This component has the following properties:

- `icon` accepts icon keywords and URLs, such as `jam:write-f`, `ic:round-home`, etc.
- `color` accepts CSS-style color values, such as `#fff`, `red`, etc. (This option is only valid for SVG icons)
- `size` accepts CSS-style sizes, such as `1rem`, `2em`, `100px`, etc.

::: demo-wrapper Example

Input:

```markdown
- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />
```

Output:

- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />

:::

### Obtaining Icon Keywords

The icons used in this documentation come from [iconify](https://iconify.design/). You can search for the icons you want on its [icon search interface](https://icon-sets.iconify.design/) and then copy their keywords.

## Frontmatter

Frontmatter is a section at the beginning of a Markdown document wrapped in `---`, using YAML syntax internally. Through Frontmatter, we can identify the document's editing time, icons used, categories, tags, etc.

::: details Example

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# Document Title

...
```

:::

The meanings of each field are as follows:

- `date`: Document editing time
- `icon`: Icon next to the document title
- `order`: Sorting of the document in the sidebar
